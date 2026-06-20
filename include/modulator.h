#ifndef MODULATOR_H
#define MODULATOR_H

#define SYMBOL_RATE     6000
#define SAMPLES_PER_SYM 8
#define SAMP_RATE       (SYMBOL_RATE * SAMPLES_PER_SYM)

#include "utils.h"

void hcpm_modulate(const int *bits, int num_bits, complex *iq, int *num_samples);

#endif
