# Walkthrough - H-CPM Modem Correction

This document details the changes and verification results for the corrected P25 Phase 2 H-CPM modem.

## Changes Made

### 1. Modulator (`src/modulator.c`)
- **Corrected Symbol Mapping (`dibit_to_level`)**: Restored mapping of `01` to `-1` and `10` to `+1` to comply with the standard.
- **Fixed Total Sample Length**: Set `total_samp` to cover the entire burst size (`total_symbols * SAMPLES_PER_SYM`) instead of just `SAMPLES_PER_SYM`.
- **Integrated Phase Pulse Scaling**: Scaled the prefix-sum in `generate_q` by `dt` to properly compute the numerical integration of the pulse $q(t)$ (using right-point rectangular integration).
- **Corrected Modulation Phase Scaling**: Added the missing scaling factor of $2\pi h$ to the phase calculation in `hcpm_modulate`.

### 2. Viterbi Decoder (`src/viterbi.c`)
- **Preamble and Tail offsets**: Set the starting sample offset to skip the preamble (`2 * SAMPLES_PER_SYM`) and run the trellis for `num_symbols + TAIL_SYMBOLS` steps.
- **Decision Feedback Sequence Estimation (DFSE)**: For the 12-state trellis to handle $L=3$ pulse memory, the predecessor of each state is traced from the trellis history to extract the symbol from two steps ago ($\alpha_k$).
- **Folded Phase State Transitions**: Implemented the folded 3-state phase transitions $J_k \to J_{k+1}$ based on $\alpha_k$ (predecessor state's symbol) using alternating odd/even step offsets.
- **Branch Metric Phase Reconstruction**: Implemented the exact continuous phase equation for each sample using the reconstructed phase state and the three active symbols.
- **Tail and Traceback**: Restricted transitions to the tail symbol $-3$ during tail periods and successfully traced back starting from the end of the tail.

---

## Verification Results

### A) Modulator Verification
The corrected modulator was built and compared against the reference bin file `ref/ref_iq_allzero.bin` (200 zero bits, mapping to 840 complex samples):
```bash
Written 840 samples to my_output.bin
Samples compared: 1680
RMS error: 5.64383090e-09
Result: Modulator MATCHES reference (correct).
```
The RMS error of **$5.64 \times 10^{-9}$** is far below the **$10^{-5}$** pass threshold.

### B) Noiseless Self-Test
The self-test executes with a fixed pattern of 32 bits (16 symbols) and decodes them in a noiseless channel:
```
Running noiseless self-test...
Self-test PASSED.
```

### C) BER Performance
The BER results printed by the test suite are:
- **$9.0\text{ dB} \implies \text{BER} = 0.004000$** (specification requires $\text{BER} \approx 10^{-2}$ at $9.2\text{ dB}$)
- **$10.0\text{ dB} \implies \text{BER} = 0.000000$** (specification requires $\text{BER} \approx 10^{-3}$ at $10.8\text{ dB}$)

All pass criteria have been successfully met.
