#include "utils.h"
#include <math.h>

complex complex_mul(complex a, complex b) {
    complex r;
    r.i = a.i * b.i - a.q * b.q;
    r.q = a.i * b.q + a.q * b.i;
    return r;
}

complex complex_exp(float phase) {
    complex r;
    r.i = cosf(phase);
    r.q = sinf(phase);
    return r;
}

float complex_phase(complex c) {
    return atan2f(c.q, c.i);
}
