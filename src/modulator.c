#include "modulator.h"
#include <math.h>
#include <string.h>

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
        /* Bug fix: Scale integration step by dt to properly compute the continuous phase pulse q(t) */
        sum += g[n] * dt;
        q[n] = sum;
    }
}

static int dibit_to_level(const int *bits, int idx) {
    int b1 = bits[2*idx];
    int b0 = bits[2*idx+1];
    /* Bug fix: Correct symbol mapping to match the specification: 00 -> -3, 01 -> -1, 10 -> +1, 11 -> +3 */
    if (b1 == 0 && b0 == 0) return -3;
    if (b1 == 0 && b0 == 1) return -1;
    if (b1 == 1 && b0 == 0) return 1;
    return 3;
}

void hcpm_modulate(const int *bits, int num_bits, complex *iq, int *num_samples) {
    int num_data_syms = num_bits / 2;
    int preamble_len = 2;
    int tail_len = 3;
    int total_symbols = preamble_len + num_data_syms + tail_len;

    int q_len = (3 + 1) * SAMPLES_PER_SYM;
    float g[q_len], q[q_len];
    memset(g, 0, sizeof(g));
    memset(q, 0, sizeof(q));

    generate_g(g, SAMPLES_PER_SYM);
    float dt = 1.0f / SAMP_RATE;
    generate_q(q, q_len, g, dt);

    float h = 1.0f / 3.0f;
    /* Bug fix: Total sample count should cover all symbols (preamble + data + tail) times samples per symbol */
    int total_samp = total_symbols * SAMPLES_PER_SYM;
    *num_samples = total_samp;

    int symbols[total_symbols];
    for (int i = 0; i < preamble_len; i++) symbols[i] = -3;
    for (int i = 0; i < num_data_syms; i++) symbols[preamble_len + i] = dibit_to_level(bits, i);
    for (int i = 0; i < tail_len; i++) symbols[preamble_len + num_data_syms + i] = -3;

    for (int n = 0; n < total_samp; n++) {
        float phi = 0.0f;
        for (int k = 0; k < total_symbols; k++) {
            int idx = n - k * SAMPLES_PER_SYM;
            if (idx >= 0) {
                if (idx < q_len)
                    phi += symbols[k] * q[idx];
                else
                    phi += symbols[k] * 0.5f;
            }
        }
        /* Bug fix: Scale the accumulated phase by 2 * PI * h to get the correct CPM signal phase */
        float phase = 2.0f * PI * h * phi;
        iq[n].i = cosf(phase);
        iq[n].q = sinf(phase);
    }
}
