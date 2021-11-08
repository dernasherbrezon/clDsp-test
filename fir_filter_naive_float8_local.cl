__kernel void fir_filter_process(__global const float *restrict input, __global const float *restrict taps, const unsigned int taps_len, __global float *output, const unsigned int decimation, local float* temp) {

    int output_offset = get_global_id(0) * 2;
    int input_offset = output_offset * decimation;
    __global float16 *in = (__global float16*)(input + input_offset);
    __global float16 *tap = (__global float16*)taps;

    int lid = get_local_id(0);
    int lsz = get_local_size(0);

    float8 real0 = (float8)(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    float8 imag0 = (float8)(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    // taps_len guaranteed divided by 8
    for (unsigned int j = 0; j < taps_len / 8; j++) {
        real0 += in->even * tap->even - in->odd * tap->odd;
        imag0 += in->even * tap->odd + in->odd * tap->even;

        in++;
        tap++;
    }
    temp[2 * lid] = real0.s0 + real0.s1 + real0.s2 + real0.s3 + real0.s4 + real0.s5 + real0.s6 + real0.s7;
    temp[2 * lid + 1] = imag0.s0 + imag0.s1 + imag0.s2 + imag0.s3 + imag0.s4 + imag0.s5 + imag0.s6 + imag0.s7;

    event_t ev = async_work_group_copy(&output[get_global_id(0) * 2], temp, lsz * 2, 0);
    wait_group_events(1, &ev);
}