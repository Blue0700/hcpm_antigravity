#ifndef UTILS_H
#define UTILS_H

#define PI 3.14159265358979323846

typedef struct {
    float i;
    float q;
} complex;

complex complex_mul(complex a, complex b);
complex complex_exp(float phase);
float complex_phase(complex c);

#endif
