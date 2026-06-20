#include "viterbi.h"
#include "modulator.h"
#include <math.h>
#include <float.h>
#include <string.h>

#define NUM_PHASE_STATES 3
#define NUM_SYMBOLS      4
#define NUM_STATES       12
#define TAIL_SYMBOLS     3

static int level_array[4] = {-3, -1, 1, 3};

static int phase_state_to_index(int phase) { return phase; }
static int level_to_index(int level) {
    if (level == -3) return 0;
    if (level == -1) return 1;
    if (level == 1)  return 2;
    return 3;
}

static void generate_g(float *g, int samples_per_sym) {
    int L = 3;
    int len = L * samples_per_sym;
    float T = 1.0f / SYMBOL_RATE;
    float dt = T / samples_per_sym;
    for (int n = 0; n < len; n++) {
        float t = n * dt;
        if (t <= L * T) {
            g[n] = (1.0f / (2.0f * L * T)) * (1.0f - cosf(2.0f * PI * t / (L * T)));
        } else {
            g[n] = 0.0f;
        }
    }
}

static void generate_q(float *q, int len, const float *g, float dt) {
    float sum = 0.0f;
    for (int n = 0; n < len; n++) {
        sum += g[n] * dt;
        q[n] = sum;
    }
}

static float get_actual_theta(int phase_state_idx, int k) {
    /* Folded 3-state actual phase reconstruction:
     * For even step k: actual phase is J_k * 2pi/3
     * For odd step k: actual phase is J_k * 2pi/3 + pi/3 */
    if (k % 2 == 0) {
        return phase_state_idx * (2.0f * PI / 3.0f);
    } else {
        return phase_state_idx * (2.0f * PI / 3.0f) + (PI / 3.0f);
    }
}

void viterbi_demodulate(const float *iq_i, const float *iq_q, int num_samples,
                        int *bits_out, int num_bits) {
    int num_symbols = num_bits / 2;
    int total_trellis_steps = num_symbols + TAIL_SYMBOLS;

    int q_len = (3 + 1) * SAMPLES_PER_SYM;
    float g[q_len], q[q_len];
    memset(g, 0, sizeof(g));
    memset(q, 0, sizeof(q));
    generate_g(g, SAMPLES_PER_SYM);
    float dt = 1.0f / SAMP_RATE;
    generate_q(q, q_len, g, dt);

    float path_metric[NUM_STATES];
    int prev_state[NUM_STATES * total_trellis_steps];
    int prev_input[NUM_STATES * total_trellis_steps];
    float h = 1.0f/3.0f;

    /* Bug fix: Force starting at state 0 since preamble is all-zero (-3, -3) and starting phase is 0 */
    path_metric[0] = 0.0f;
    for (int s = 1; s < NUM_STATES; s++) path_metric[s] = INFINITY;

    /* Transition offsets for folded 3-state phase transitions */
    int offset_even[4] = {1, 2, 0, 1};
    int offset_odd[4] = {2, 0, 1, 2};

    /* Bug fix: Skip the preamble (2 symbols = 16 samples) */
    int sample_offset = 2 * SAMPLES_PER_SYM;

    for (int k = 0; k < total_trellis_steps; k++) {
        float new_metrics[NUM_STATES];
        for (int s = 0; s < NUM_STATES; s++) new_metrics[s] = INFINITY;

        for (int s = 0; s < NUM_STATES; s++) {
            if (path_metric[s] == INFINITY) continue;

            int old_phase = s / 4;
            int old_prev_sym_idx = s % 4;
            int old_prev_sym = level_array[old_prev_sym_idx];

            /* Bug fix (Predecessor tracking & DFSE): 
             * Retrieve the symbol from two steps ago (alpha_k) using predecessor state info for k >= 2.
             * For k < 2, the symbol is the preamble symbol -3. */
            int alpha_k_val = -3;
            if (k >= 2) {
                int s_prev = prev_state[s * total_trellis_steps + k - 1];
                alpha_k_val = level_array[s_prev % 4];
            }

            float theta_k = get_actual_theta(old_phase, k);

            for (int inp = 0; inp < 4; inp++) {
                /* Bug fix (Tail restraints): 
                 * During tail symbols (k >= num_symbols), only allow transitions for the tail symbol -3 (inp = 0) */
                if (k >= num_symbols && inp != 0) continue;

                int sym = level_array[inp];

                float metric = 0.0f;
                for (int n = 0; n < SAMPLES_PER_SYM; n++) {
                    int idx = sample_offset + n;
                    if (idx >= num_samples) break;
                    float rx_i = iq_i[idx];
                    float rx_q = iq_q[idx];

                    /* Bug fix (Branch metric computation): 
                     * Compute correct ideal phase using full phase equation with phase state and three active symbols */
                    float phase = theta_k + 2.0f * PI * h * (alpha_k_val * q[16 + n] + old_prev_sym * q[8 + n] + sym * q[n]);
                    float ideal_i = cosf(phase);
                    float ideal_q = sinf(phase);

                    float diff_i = rx_i - ideal_i;
                    float diff_q = rx_q - ideal_q;
                    metric += diff_i * diff_i + diff_q * diff_q;
                }

                /* Bug fix (State transitions): 
                 * Phase transitions must use the folded 3-state phase update based on alpha_k_val, with alternating step parity */
                int alpha_k_idx = level_to_index(alpha_k_val);
                int new_phase_state;
                if (k % 2 == 0) {
                    new_phase_state = (old_phase + offset_even[alpha_k_idx]) % 3;
                } else {
                    new_phase_state = (old_phase + offset_odd[alpha_k_idx]) % 3;
                }
                int new_prev_sym_idx = inp;
                int new_state = new_phase_state * 4 + new_prev_sym_idx;

                float total_metric = path_metric[s] + metric;
                if (total_metric < new_metrics[new_state]) {
                    new_metrics[new_state] = total_metric;
                    prev_state[new_state * total_trellis_steps + k] = s;
                    prev_input[new_state * total_trellis_steps + k] = inp;
                }
            }
        }

        for (int s = 0; s < NUM_STATES; s++) path_metric[s] = new_metrics[s];
        sample_offset += SAMPLES_PER_SYM;
    }

    /* Trace back from the end of the trellis (including tail) */
    int best_state = 0;
    float best_metric = path_metric[0];
    for (int s = 1; s < NUM_STATES; s++) {
        if (path_metric[s] < best_metric) {
            best_metric = path_metric[s];
            best_state = s;
        }
    }

    /* Bug fix (Traceback operation): 
     * Trace back through tail symbols first, then decode data symbols */
    for (int k = total_trellis_steps - 1; k >= num_symbols; k--) {
        best_state = prev_state[best_state * total_trellis_steps + k];
    }

    for (int k = num_symbols - 1; k >= 0; k--) {
        int inp = prev_input[best_state * total_trellis_steps + k];
        /* Bug fix (Symbol decisions & bit ordering):
         * Correctly decode symbols and place them in bits_out array */
        if (inp == 0) { bits_out[2*k] = 0; bits_out[2*k+1] = 0; }
        else if (inp == 1) { bits_out[2*k] = 0; bits_out[2*k+1] = 1; }
        else if (inp == 2) { bits_out[2*k] = 1; bits_out[2*k+1] = 0; }
        else { bits_out[2*k] = 1; bits_out[2*k+1] = 1; }
        best_state = prev_state[best_state * total_trellis_steps + k];
    }
}