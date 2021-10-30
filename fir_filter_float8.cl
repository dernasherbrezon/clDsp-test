__kernel void fir_filter_process(__global const float *restrict input, __global const float *restrict taps, const unsigned int taps_len, __global float *output, const unsigned int decimation, const unsigned int output_len) {

    for (unsigned int i = 0; i < output_len; i++) {
        int output_offset = (get_global_id(0) * output_len + i) * 2;
        int input_offset = output_offset * decimation;
        __global float8 *in = (__global float8*)(input + input_offset);
        __global float8 *tap = (__global float8*)taps;
        float real0 = 0.0f;
        float imag0 = 0.0f;
        float real1 = 0.0f;
        float imag1 = 0.0f;
        float real2 = 0.0f;
        float imag2 = 0.0f;
        float real3 = 0.0f;
        float imag3 = 0.0f;
        // taps_len guaranteed divided by 4
        for (unsigned int j = 0; j < taps_len / 4; j++) {
            real0 += (in->s0 * tap->s0) - (in->s1 * tap->s1);
            imag0 += (in->s0 * tap->s1) + (in->s1 * tap->s0);

            real1 += (in->s2 * tap->s2) - (in->s3 * tap->s3);
            imag1 += (in->s2 * tap->s3) + (in->s3 * tap->s2);

            real2 += (in->s4 * tap->s4) - (in->s5 * tap->s5);
            imag2 += (in->s4 * tap->s5) + (in->s5 * tap->s4);

            real3 += (in->s6 * tap->s6) - (in->s7 * tap->s7);
            imag3 += (in->s6 * tap->s7) + (in->s7 * tap->s6);

            in++;
            tap++;
        }
        output[output_offset] = real0 + real1 + real2 + real3;
        output[output_offset + 1] = imag0 + imag1 + imag2 + imag3;
    }

}