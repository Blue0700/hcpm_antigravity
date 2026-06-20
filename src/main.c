#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "modulator.h"
#include "demodulator.h"
#include "channel.h"

#define MAX_SAMPS 20000

int main() {
    srand(100);

    printf("Running noiseless self-test...\n");
    int test_bits[] = {
        0,0, 0,1, 1,0, 1,1, 0,0, 0,1, 1,0, 1,1,
        0,0, 0,1, 1,0, 1,1, 0,0, 0,1, 1,0, 1,1
    };
    int num_test_bits = sizeof(test_bits) / sizeof(test_bits[0]);

    complex iq_test[MAX_SAMPS];
    int nsamp_test;
    hcpm_modulate(test_bits, num_test_bits, iq_test, &nsamp_test);

    float i_arr_test[MAX_SAMPS], q_arr_test[MAX_SAMPS];
    for (int n = 0; n < nsamp_test; n++) {
        i_arr_test[n] = iq_test[n].i;
        q_arr_test[n] = iq_test[n].q;
    }

    int rx_test_bits[num_test_bits];
    hcpm_demodulate(i_arr_test, q_arr_test, nsamp_test, rx_test_bits, num_test_bits);

    int errors_test = 0;
    for (int i = 0; i < num_test_bits; i++)
        if (test_bits[i] != rx_test_bits[i]) errors_test++;

    if (errors_test > 0) {
        printf("Self-test FAILED: %d bit errors\n", errors_test);
        return 1;
    }
    printf("Self-test PASSED.\n\n");

    printf("Eb/N0(dB)\tBER\n");
    for (float ebn0_db = 4.0f; ebn0_db <= 14.0f; ebn0_db += 1.0f) {
        int num_bits = 2000;
        int bits[num_bits];
        for (int i = 0; i < num_bits; i++) bits[i] = rand() % 2;

        complex iq[MAX_SAMPS];
        int nsamp;
        hcpm_modulate(bits, num_bits, iq, &nsamp);

        float ebn0_lin = powf(10.0f, ebn0_db / 10.0f);
        float noise_var = 2.0f / ebn0_lin;
        add_awgn(iq, nsamp, noise_var);

        float i_arr[MAX_SAMPS], q_arr[MAX_SAMPS];
        for (int n = 0; n < nsamp; n++) {
            i_arr[n] = iq[n].i;
            q_arr[n] = iq[n].q;
        }

        int rx_bits[num_bits];
        hcpm_demodulate(i_arr, q_arr, nsamp, rx_bits, num_bits);

        int errors = 0;
        for (int i = 0; i < num_bits; i++)
            if (bits[i] != rx_bits[i]) errors++;
        float ber = (float)errors / num_bits;
        printf("%.1f\t\t%.6f\n", ebn0_db, ber);
    }
    return 0;
}
