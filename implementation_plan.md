# Implementation Plan - H-CPM Modem Correction

This plan outlines the fixes for the H-CPM modulator (`src/modulator.c`) and the Viterbi decoder (`src/viterbi.c`) to satisfy all the P25 Phase 2 specifications, including the RMS error, noiseless self-test, and BER requirements.

## User Review Required

> [!IMPORTANT]
> The Viterbi decoder uses a 12-state trellis, which corresponds to 3 phase states and 4 previous symbol levels (since $L=3$). To resolve the phase dependency on the symbol from two steps ago ($\alpha_k$), the decoder will use **Decision Feedback Sequence Estimation (DFSE)** by tracing the predecessor of each state in the trellis.

## Proposed Changes

### 1. Modulator
File: [modulator.c](file:///c:/Users/amali/Music/Evaluation-Questions/Q2-HCPM/source/src/modulator.c)

#### Bug Fixes:
1. **Symbol Mapping (`dibit_to_level`)**: Correct the mapping of `01` and `10` to match the specification:
   - `01` $\to -1$ (was $+1$ in code)
   - `10` $\to +1$ (was $-1$ in code)
2. **Total Sample Count**: Correct `total_samp` to cover the entire burst:
   - `total_samp = total_symbols * SAMPLES_PER_SYM` (was only `SAMPLES_PER_SYM` in code)
3. **Phase Pulse Integration (`generate_q`)**: Change the integration step to scale by `dt` to match the right-point rectangular rule, which matches the reference IQ output:
   - `sum += g[n] * dt` (was `sum += g[n]`)
4. **Phase Scaling in Modulation (`hcpm_modulate`)**: Scale the phase by $2\pi h$ before applying cosine/sine:
   - `float phase = 2.0f * PI * h * phi;`
   - `iq[n].i = cosf(phase); iq[n].q = sinf(phase);` (was missing the scaling factor)

---

### 2. Viterbi Decoder
File: [viterbi.c](file:///c:/Users/amali/Music/Evaluation-Questions/Q2-HCPM/source/src/viterbi.c)

#### Bug Fixes:
1. **Preamble and Tail Offsets**: Set `sample_offset` to start after the preamble (`2 * SAMPLES_PER_SYM`) and run the trellis loop for both data and tail symbols:
   - `total_trellis_steps = num_symbols + TAIL_SYMBOLS`
2. **Branch Metric Phase Equation**: Compute the exact H-CPM phase at sample $n$ using:
   $$\phi[n] = \theta_k + 2\pi h \left( \alpha_k q[16+n] + \alpha_{k+1} q[8+n] + \alpha_{k+2} q[n] \right)$$
   where:
   - $\alpha_{k+2}$ is the current transition symbol `sym`
   - $\alpha_{k+1}$ is the previous symbol `old_prev_sym` (from the state $s$)
   - $\alpha_k$ is retrieved from the survivor path predecessor of state $s$ (for $k \ge 2$) or set to $-3$ (for $k < 2$).
3. **Folded Phase State Transitions**: Maintain the folded 3-state phase index $J_k \in \{0, 1, 2\}$:
   - Parity of phase states alternates with step parity:
     - For even step $k$: $J_{k+1} = \left( J_k + \frac{\alpha_k - 1}{2} \right) \bmod 3$
     - For odd step $k$: $J_{k+1} = \left( J_k + \frac{\alpha_k + 1}{2} \right) \bmod 3$
   - Transition offsets are mapped as:
     - Even step $k$: `offset = {1, 2, 0, 1}` for symbol indices `{0, 1, 2, 3}`
     - Odd step $k$: `offset = {2, 0, 1, 2}` for symbol indices `{0, 1, 2, 3}`
   - The phase state is updated using `old_prev_sym` ($\alpha_k$), not the input `sym`.
4. **Decoder Phase Reconstruction**:
   - Even step $k$: $\theta_k = J_k \times \frac{2\pi}{3}$
   - Odd step $k$: $\theta_k = J_k \times \frac{2\pi}{3} + \frac{\pi}{3}$
5. **Path Metric Initialization**: Only state 0 should start with `0.0f`; all other states must start with `INFINITY` to ensure synchronization with the preamble.
6. **Tail Restraints**: During tail symbols ($k \ge \text{num\_symbols}$), restrict transitions to the known tail symbol $-3$ (`inp == 0`).
7. **Traceback and Bit Decoding**: Trace back from the end of the tail to get the data symbols, mapping them to bits.

## Verification Plan

### Automated Tests
- Build and run the mod comparator:
  `gcc -std=c99 -O2 -Iinclude -o generate_iq tools/generate_iq.c src/modulator.c src/utils.c -lm`
  `./generate_iq`
  `gcc -O2 -o compare tools/compare.c -lm`
  `./compare ref/ref_iq_allzero.bin my_output.bin`
  Verify that RMS error is $< 10^{-5}$.
- Build and run the self-test and BER measurement suite:
  `mingw32-make`
  `./hcpm_test`
  Verify that noiseless test passes with 0 bit errors and the BER requirements are met.
