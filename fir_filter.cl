__kernel void fir_filter_process(__global const float *restrict input, __global const float *restrict taps, const unsigned int taps_len, __global float *output, const unsigned int decimation, const unsigned int output_len) {

    for (unsigned int i = 0; i < output_len; i++) {
        int output_offset = (get_global_id(0) * output_len + i) * 2;
        int input_offset = output_offset * decimation;
        float real0 = 0.0f;
        float imag0 = 0.0f;
        for (unsigned int j = 0; j < taps_len; j++) {
            real0 += (input[input_offset + 2 * j] * taps[2 * j]) - (input[input_offset + 2 * j + 1] * taps[2 * j + 1]);
            imag0 += (input[input_offset + 2 * j] * taps[2 * j + 1]) + (input[input_offset + 2 * j + 1] * taps[2 * j]);
        }
        output[output_offset] = real0;
        output[output_offset + 1] = imag0;
    }

}