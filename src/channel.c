#include "channel.h"
#include <stdlib.h>
#include <math.h>

float awgn_noise(float variance) {
    float u1 = (float)rand() / RAND_MAX;
    float u2 = (float)rand() / RAND_MAX;
    float r = sqrtf(-2.0f * variance * logf(u1 + 1e-12f));
    return r * cosf(2.0f * PI * u2);
}

void add_awgn(complex *sig, int len, float variance) {
    for (int n = 0; n < len; n++) {
        sig[n].i += awgn_noise(variance);
        sig[n].q += awgn_noise(variance);
    }
}
