#ifndef DEMODULATOR_H
#define DEMODULATOR_H

void hcpm_demodulate(const float *iq_i, const float *iq_q, int num_samples,
                     int *bits_out, int num_bits);

#endif
