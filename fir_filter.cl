#define DECIMATION 42
#define TAPS_SIZE 2429
#define WORK_ITEMS 2381
//#define TAPS_SIZE 1
//#define DECIMATION 2

__kernel void fir_filter_process(__global const float *restrict A, __global const float *restrict B, __global float *C) {

    int result_offset = get_global_id(0) * 2;
    int quads = TAPS_SIZE / 4;
    float real0 = 0.0f;
    float imag0 = 0.0f;
    float real1 = 0.0f;
    float imag1 = 0.0f;
    float real2 = 0.0f;
    float imag2 = 0.0f;
    float real3 = 0.0f;
    float imag3 = 0.0f;

//    real += (A[0] * B[0]) - (A[1] * B[1]);
//    imag += (A[0] * B[1]) + (A[1] * B[0]);
//
//    real += (A[2] * B[2]) - (A[3] * B[3]);
//    imag += (A[2] * B[3]) + (A[3] * B[2]);

//    real += (A[0] * B[0]) - (A[1] * B[1]);
//    real += B[0];
//    imag += -0.0000060;

//    real += -0.0000050;
//    imag += -0.0000270;
    int offset = get_global_id(0) * 2 * DECIMATION;

    for (int i = 0; i < quads; i++) {
//        real += (A[offset + i] * B[i]);
//        real -= (A[offset + i + 1] * B[i + 1]);
//        imag += (A[offset + i] * B[i + 1]);
//        imag += (A[offset + i + 1] * B[i]);
        real0 += (A[offset + i * 2 * 4] * B[i * 2 * 4]) - (A[offset + i * 2 * 4 + 1] * B[i * 2 * 4 + 1]);
        imag0 += (A[offset + i * 2 * 4] * B[i * 2 * 4 + 1]) + (A[offset + i * 2 * 4 + 1] * B[i * 2 * 4]);

        real1 += (A[offset + i * 2 * 4 + 2] * B[i * 2 * 4 + 2]) - (A[offset + i * 2 * 4 + 3] * B[i * 2 * 4 + 3]);
        imag1 += (A[offset + i * 2 * 4 + 2] * B[i * 2 * 4 + 3]) + (A[offset + i * 2 * 4 + 3] * B[i * 2 * 4 + 2]);

        real2 += (A[offset + i * 2 * 4 + 4] * B[i * 2 * 4 + 4]) - (A[offset + i * 2 * 4 + 5] * B[i * 2 * 4 + 5]);
        imag2 += (A[offset + i * 2 * 4 + 4] * B[i * 2 * 4 + 5]) + (A[offset + i * 2 * 4 + 5] * B[i * 2 * 4 + 4]);

        real3 += (A[offset + i * 2 * 4 + 6] * B[i * 2 * 4 + 6]) - (A[offset + i * 2 * 4 + 7] * B[i * 2 * 4 + 7]);
        imag3 += (A[offset + i * 2 * 4 + 6] * B[i * 2 * 4 + 7]) + (A[offset + i * 2 * 4 + 7] * B[i * 2 * 4 + 6]);
    }

    for (int i = quads * 2 * 4; i < TAPS_SIZE * 2; i += 2) {
        real0 += (A[offset + i] * B[i]) - (A[offset + i + 1] * B[i + 1]);
        imag0 += (A[offset + i] * B[i + 1]) + (A[offset + i + 1] * B[i]);
    }

    C[result_offset] = real0 + real1 + real2 + real3;
    C[result_offset + 1] = imag0 + imag1 + imag2 + imag3;

}