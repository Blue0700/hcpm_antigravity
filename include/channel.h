#ifndef CHANNEL_H
#define CHANNEL_H

#include "utils.h"

float awgn_noise(float variance);
void add_awgn(complex *sig, int len, float variance);

#endif
