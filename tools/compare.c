#include <stdio.h>
#include <math.h>

int main(int argc, char **argv) {
    const char *ref_file = "ref/ref_iq_allzero.bin";
    const char *test_file = "my_output.bin";

    if (argc == 3) { ref_file = argv[1]; test_file = argv[2]; }

    FILE *fr = fopen(ref_file, "rb");
    FILE *ft = fopen(test_file, "rb");
    if (!fr || !ft) { printf("Cannot open files.\n"); return 1; }

    float r, t;
    double sum_sq = 0.0;
    long n = 0;

    while (fread(&r, sizeof(float), 1, fr) == 1 &&
           fread(&t, sizeof(float), 1, ft) == 1) {
        double diff = r - t;
        sum_sq += diff * diff;
        n++;
    }
    fclose(fr); fclose(ft);

    if (n == 0) { printf("File empty or lengths differ.\n"); return 1; }

    double rms = sqrt(sum_sq / n);
    printf("Samples compared: %ld\n", n);
    printf("RMS error: %.8e\n", rms);

    if (rms < 1e-5)
        printf("Result: Modulator MATCHES reference (correct).\n");
    else
        printf("Result: Modulator DOES NOT MATCH reference (still buggy).\n");

    return 0;
}
