#include "fir_filter.h"
#include <errno.h>
#include <string.h>
#include <complex.h>
#include <stdio.h>
#include <math.h>

#define CL_TARGET_OPENCL_VERSION 120

#include <CL/cl.h>

#define MAX_SOURCE_SIZE (0x100000)

struct fir_filter_t {
    uint8_t decimation;

    float complex *taps;
    size_t taps_len;

    float complex *working_buffer;
    size_t history_offset;
    size_t working_len_total;
    size_t max_input_buffer_length;

    float complex *output;
    size_t output_len;

    cl_mem c_mem_obj;
    cl_mem b_mem_obj;
    cl_mem a_mem_obj;
    cl_command_queue command_queue;
    cl_kernel kernel;
    cl_context context;
    cl_device_id device_id;
    cl_program program;
};

int fir_filter_create(uint8_t decimation, float complex *taps, size_t taps_len, size_t max_input_buffer_length, fir_filter **filter) {
    struct fir_filter_t *result = malloc(sizeof(struct fir_filter_t));
    if (result == NULL) {
        return -ENOMEM;
    }
    // init all fields with 0 so that destroy_* method would work
    *result = (struct fir_filter_t) {0};

    result->decimation = decimation;
    result->taps_len = taps_len;
    result->taps = taps;
    result->history_offset = taps_len - 1;
    result->max_input_buffer_length = max_input_buffer_length;

    // +1 for case when round-up needed.
    size_t max_input_with_history = max_input_buffer_length + result->history_offset;
    result->output_len = (max_input_with_history - taps_len) / decimation + 1;
    printf("output length max: %zu\n", result->output_len);
    // align to 12x GPU cores in raspberry pi
    result->output_len = (size_t) ceilf((float) result->output_len / 12) * 12;
    printf("output length max: %zu\n", result->output_len);
    result->output = malloc(sizeof(float complex) * result->output_len);
    if (result->output == NULL) {
        fir_filter_destroy(result);
        return -ENOMEM;
    }
    memset(result->output, 0, sizeof(float complex) * result->output_len);

    //output_len is rounded to 12, thus working_buffer
    result->working_len_total = (result->output_len - 1) * decimation + taps_len;
    printf("working_len_total: %zu\n", result->working_len_total);
    result->working_buffer = malloc(sizeof(float complex) * result->working_len_total);
    if (result->working_buffer == NULL) {
        fir_filter_destroy(result);
        return -ENOMEM;
    }
    memset(result->working_buffer, 0, sizeof(float complex) * result->working_len_total);

    FILE *fp;
    char *source_str;
    size_t source_size;

    fp = fopen("../fir_filter.cl", "r");
    if (!fp) {
        fprintf(stderr, "Failed to load kernel.\n");
        exit(1);
    }
    source_str = (char *) malloc(MAX_SOURCE_SIZE);
    source_size = fread(source_str, 1, MAX_SOURCE_SIZE, fp);
    fclose(fp);

    // Get platform and device information
    cl_platform_id platform_id = NULL;
    result->device_id = NULL;
    cl_uint ret_num_devices;
    cl_uint ret_num_platforms;
    cl_int ret = clGetPlatformIDs(1, &platform_id, &ret_num_platforms);
    ret = clGetDeviceIDs(platform_id, CL_DEVICE_TYPE_ALL, 1,
                         &result->device_id, &ret_num_devices);
    printf("clGetDeviceIDs: %d\n", ret);
    // Create an OpenCL context
    result->context = clCreateContext(NULL, 1, &result->device_id, NULL, NULL, &ret);
    printf("clCreateContext: %d\n", ret);

    // Create a command queue
    result->command_queue = clCreateCommandQueue(result->context, result->device_id, 0, &ret);
    printf("clCreateCommandQueue: %d\n", ret);

// Create memory buffers on the device for each vector
    result->a_mem_obj = clCreateBuffer(result->context, CL_MEM_READ_ONLY,
                                       result->working_len_total * sizeof(float complex), NULL, &ret);
    printf("clCreateBuffer: %d\n", ret);
    result->b_mem_obj = clCreateBuffer(result->context, CL_MEM_READ_ONLY,
                                       taps_len * sizeof(float complex), NULL, &ret);
    printf("clCreateBuffer: %d\n", ret);
    result->c_mem_obj = clCreateBuffer(result->context, CL_MEM_WRITE_ONLY,
                                       result->output_len * sizeof(float complex), NULL, &ret);
    printf("clCreateBuffer: %d\n", ret);

    // Copy the lists A and B to their respective memory buffers
    ret = clEnqueueWriteBuffer(result->command_queue, result->b_mem_obj, CL_TRUE, 0,
                               taps_len * sizeof(float complex), result->taps, 0, NULL, NULL);
    printf("clEnqueueWriteBuffer B: %d\n", ret);

    // Create a program from the kernel source
    result->program = clCreateProgramWithSource(result->context, 1,
                                                (const char **) &source_str, (const size_t *) &source_size, &ret);
    printf("clCreateProgramWithSource: %d\n", ret);

    // Build the program
    ret = clBuildProgram(result->program, 1, &result->device_id, NULL, NULL, NULL);
    printf("clBuildProgram: %d\n", ret);

    // Create the OpenCL kernel
    result->kernel = clCreateKernel(result->program, "fir_filter_process", &ret);
    printf("clCreateKernel: %d\n", ret);

    // Set the arguments of the kernel
    ret = clSetKernelArg(result->kernel, 0, sizeof(cl_mem), (void *) &result->a_mem_obj);
    printf("clSetKernelArg: %d\n", ret);
    ret = clSetKernelArg(result->kernel, 1, sizeof(cl_mem), (void *) &result->b_mem_obj);
    printf("clSetKernelArg: %d\n", ret);
    ret = clSetKernelArg(result->kernel, 2, sizeof(cl_mem), (void *) &result->c_mem_obj);
    printf("clSetKernelArg: %d\n", ret);

    *filter = result;
    return 0;
}

void fir_filter_process(const float complex *input, size_t input_len, float complex **output, size_t *output_len, fir_filter *filter) {
    if (input_len > filter->max_input_buffer_length) {
        fprintf(stderr, "<3>requested buffer %zu is more than max: %zu\n", input_len, filter->max_input_buffer_length);
        *output = NULL;
        *output_len = 0;
        return;
    }

    memcpy(filter->working_buffer + filter->history_offset, input, sizeof(float complex) * input_len);
    size_t working_len = filter->history_offset + input_len;
    size_t current_index = 0;
    // input might not have enough data to produce output sample
//    size_t global_item_size = 2381; // Process the entire lists
    size_t global_item_size = (working_len - filter->taps_len) / filter->decimation + 1;
    printf("total output items: %zu\n", global_item_size);
    size_t global_item_size_rounded = (size_t) ceilf((float) global_item_size / 12) * 12;
    printf("total output items: %zu\n", global_item_size_rounded);
    size_t local_item_size = 12;
    size_t work_items = 12;
    current_index = global_item_size * filter->decimation;
//    size_t local_item_size = 2; // Process in groups of 64
    if (working_len > (filter->taps_len - 1)) {
        cl_int ret = clEnqueueWriteBuffer(filter->command_queue, filter->a_mem_obj, CL_TRUE, 0,
                                          working_len * sizeof(float complex), filter->working_buffer, 0, NULL, NULL);
        printf("clEnqueueWriteBuffer data: %d\n", ret);
        ret = clEnqueueNDRangeKernel(filter->command_queue, filter->kernel, 1, NULL,
                                     &work_items, &local_item_size, 0, NULL, NULL);
        printf("clEnqueueNDRangeKernel: %d\n", ret);
        ret = clEnqueueReadBuffer(filter->command_queue, filter->c_mem_obj, CL_TRUE, 0,
                                  global_item_size * sizeof(float complex), filter->output, 0, NULL, NULL);
        printf("clEnqueueReadBuffer: %d\n", ret);
//        produced = 2381;

//        int offset = 100;
//        for (int i = 0; i < filter->taps_len * 2; i += 2) {
//            printf("%.9fF,%.9fF, ", filter->working_buffer[offset * 2 * filter->decimation + i], filter->working_buffer[offset * 2 * filter->decimation + i + 1]);
//        }
//        printf("\n");
//        for (int i = 0; i < filter->taps_len; i++) {
//            printf("%.9fF,%.9fF, ", crealf(filter->taps[0][i]), cimagf(filter->taps[0][i]));
//        }
//        printf("\n");
//        for (int i = 0; i < 20; i++) {
//            printf("%.9f,%.9f, ", crealf(filter->output[offset + i]), cimagf(filter->output[offset + i]));
//        }
//        printf("\n");

    }
    // preserve history for the next execution
    filter->history_offset = working_len - current_index;
    if (current_index > 0) {
        memmove(filter->working_buffer, filter->working_buffer + current_index, sizeof(float complex) * filter->history_offset);
    }

    *output = filter->output;
    *output_len = global_item_size;
}

void fir_filter_destroy(fir_filter *filter) {
    if (filter == NULL) {
        return;
    }
    if (filter->taps != NULL) {
        free(filter->taps);
    }
    if (filter->output != NULL) {
        free(filter->output);
    }
    if (filter->working_buffer != NULL) {
        free(filter->working_buffer);
    }
    if (filter->command_queue != NULL) {
        cl_int ret = clFlush(filter->command_queue);
        ret = clFinish(filter->command_queue);
    }
    if (filter->kernel != NULL) {
        cl_int ret = clReleaseKernel(filter->kernel);
    }
    if (filter->program != NULL) {
        cl_int ret = clReleaseProgram(filter->program);
    }
    if (filter->a_mem_obj != NULL) {
        cl_int ret = clReleaseMemObject(filter->a_mem_obj);
    }
    if (filter->b_mem_obj != NULL) {
        cl_int ret = clReleaseMemObject(filter->b_mem_obj);
    }
    if (filter->c_mem_obj != NULL) {
        cl_int ret = clReleaseMemObject(filter->c_mem_obj);
    }
    if (filter->command_queue != NULL) {
        cl_int ret = clReleaseCommandQueue(filter->command_queue);
    }
    if (filter->context != NULL) {
        cl_int ret = clReleaseContext(filter->context);
    }
    free(filter);
}
