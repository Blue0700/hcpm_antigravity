#include <stdio.h>
#include "modulator.h"

int main() {
    /* 200 zero bits → 100 data symbols, all mapping to -3 */
    int bits[200] = {0};

    complex iq[20000];
    int nsamp;

    hcpm_modulate(bits, 200, iq, &nsamp);

    FILE *f = fopen("my_output.bin", "wb");
    if (!f) {
        printf("Error: cannot open my_output.bin for writing.\n");
        return 1;
    }
    for (int n = 0; n < nsamp; n++) {
        fwrite(&iq[n].i, sizeof(float), 1, f);
        fwrite(&iq[n].q, sizeof(float), 1, f);
    }
    fclose(f);
    printf("Written %d samples to my_output.bin\n", nsamp);
    return 0;
}
