#include "demodulator.h"
#include "viterbi.h"

void hcpm_demodulate(const float *iq_i, const float *iq_q, int num_samples,
                     int *bits_out, int num_bits) {
    viterbi_demodulate(iq_i, iq_q, num_samples, bits_out, num_bits);
}
