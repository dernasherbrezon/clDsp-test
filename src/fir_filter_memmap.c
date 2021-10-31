#include "fir_filter_memmap.h"
#include <errno.h>
#include <string.h>
#include <complex.h>
#include <stdio.h>
#include <math.h>

#define CL_TARGET_OPENCL_VERSION 120

#include <CL/cl.h>

#define MAX_SOURCE_SIZE (0x100000)

struct fir_filter_memmapt {
    uint8_t decimation;

    float complex *taps;
    float complex *original_taps;
    size_t taps_len;
    size_t original_taps_len;

    float complex *working_buffer;
    size_t history_offset;
    size_t working_len_total;
    size_t max_input_buffer_length;

    float complex *output;
    size_t output_len;

    cl_mem output_obj;
    cl_mem taps_obj;
    cl_mem input_obj;
    cl_command_queue command_queue;
    cl_kernel kernel;
    cl_context context;
    cl_device_id device_id;
    cl_program program;
};

int fir_filter_memmap_create(uint8_t decimation, float complex *taps, size_t taps_len, size_t max_input_buffer_length, fir_filter_memmap **filter) {
    struct fir_filter_memmapt *result = malloc(sizeof(struct fir_filter_memmapt));
    if (result == NULL) {
        return -ENOMEM;
    }
    // init all fields with 0 so that destroy_* method would work
    *result = (struct fir_filter_memmapt) {0};

    result->decimation = decimation;
    result->original_taps_len = taps_len;
    result->taps_len = ceilf((float) taps_len / 4) * 4;
    printf("original taps: %zu rounded: %zu\n", taps_len, result->taps_len);
    result->original_taps = taps;
    result->taps = malloc(sizeof(float complex) * result->taps_len);
    if (result->taps == NULL) {
        return -ENOMEM;
    }
    memset(result->taps, 0, sizeof(float complex) * result->taps_len);
    memcpy(result->taps, taps, sizeof(float complex) * result->original_taps_len);

    result->history_offset = taps_len - 1;
    result->max_input_buffer_length = max_input_buffer_length;

    size_t max_input_with_history = max_input_buffer_length + result->history_offset;
    result->output_len = (size_t) ceilf((float) (max_input_with_history - result->taps_len) / (float) decimation);
    printf("output length max: %zu\n", result->output_len);
    // align to 12x GPU cores in raspberry pi
    result->output_len = (size_t) ceilf((float) result->output_len / 12) * 12;
    printf("output length max: %zu\n", result->output_len);
    result->output = malloc(sizeof(float complex) * result->output_len);
    if (result->output == NULL) {
        fir_filter_memmap_destroy(result);
        return -ENOMEM;
    }
    memset(result->output, 0, sizeof(float complex) * result->output_len);

    //output_len is rounded to 12, thus working_buffer
    result->working_len_total = result->output_len * decimation + result->history_offset;
    printf("working_len_total: %zu\n", result->working_len_total);
//    result->working_buffer = malloc(sizeof(float complex) * result->working_len_total);
//    if (result->working_buffer == NULL) {
//        fir_filter_memmap_destroy(result);
//        return -ENOMEM;
//    }
//    memset(result->working_buffer, 0, sizeof(float complex) * result->working_len_total);

    FILE *fp;
    char *source_str;
    size_t source_size;

    fp = fopen("../fir_filter_float8.cl", "r");
    if (!fp) {
        fprintf(stderr, "Failed to load kernel.\n");
        fir_filter_memmap_destroy(result);
        return -1;
    }
    source_str = (char *) malloc(MAX_SOURCE_SIZE);
    if (source_str == NULL) {
        fir_filter_memmap_destroy(result);
        return -ENOMEM;
    }
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
    if (ret != 0) {
        fir_filter_memmap_destroy(result);
        return ret;
    }
    // Create an OpenCL context
    result->context = clCreateContext(NULL, 1, &result->device_id, NULL, NULL, &ret);
    printf("clCreateContext: %d\n", ret);
    if (ret != 0) {
        fir_filter_memmap_destroy(result);
        return ret;
    }
    // Create a command queue
    result->command_queue = clCreateCommandQueue(result->context, result->device_id, 0, &ret);
    printf("clCreateCommandQueue: %d\n", ret);
    if (ret != 0) {
        fir_filter_memmap_destroy(result);
        return ret;
    }
// Create memory buffers on the device for each vector
    printf("allocated working buf: %zu\n", result->working_len_total);
    result->input_obj = clCreateBuffer(result->context, CL_MEM_READ_ONLY,
                                       result->working_len_total * sizeof(float complex), NULL, &ret);
    printf("clCreateBuffer: %d\n", ret);
    if (ret != 0) {
        fir_filter_memmap_destroy(result);
        return ret;
    }
    result->taps_obj = clCreateBuffer(result->context, CL_MEM_READ_ONLY,
                                      result->taps_len * sizeof(float complex), NULL, &ret);
    printf("clCreateBuffer: %d\n", ret);
    if (ret != 0) {
        fir_filter_memmap_destroy(result);
        return ret;
    }
    result->output_obj = clCreateBuffer(result->context, CL_MEM_WRITE_ONLY,
                                        result->output_len * sizeof(float complex), NULL, &ret);
    printf("clCreateBuffer: %d\n", ret);
    if (ret != 0) {
        fir_filter_memmap_destroy(result);
        return ret;
    }

    // Copy the lists A and B to their respective memory buffers
    ret = clEnqueueWriteBuffer(result->command_queue, result->taps_obj, CL_TRUE, 0,
                               result->taps_len * sizeof(float complex), result->taps, 0, NULL, NULL);
    printf("clEnqueueWriteBuffer B: %d\n", ret);
    if (ret != 0) {
        fir_filter_memmap_destroy(result);
        return ret;
    }

    // Create a program from the kernel source
    result->program = clCreateProgramWithSource(result->context, 1,
                                                (const char **) &source_str, (const size_t *) &source_size, &ret);
    printf("clCreateProgramWithSource: %d\n", ret);
    if (ret != 0) {
        fir_filter_memmap_destroy(result);
        return ret;
    }

    //FIXME sigserv when program cannot be built
    // Build the program
    ret = clBuildProgram(result->program, 1, &result->device_id, NULL, NULL, NULL);
    printf("clBuildProgram: %d\n", ret);
    if (ret != 0) {
        fir_filter_memmap_destroy(result);
        return ret;
    }

    // Create the OpenCL kernel
    result->kernel = clCreateKernel(result->program, "fir_filter_process", &ret);
    printf("clCreateKernel: %d\n", ret);
    if (ret != 0) {
        fir_filter_memmap_destroy(result);
        return ret;
    }

    // Set the arguments of the kernel
    ret = clSetKernelArg(result->kernel, 0, sizeof(cl_mem), (void *) &result->input_obj);
    printf("clSetKernelArg: %d\n", ret);
    if (ret != 0) {
        fir_filter_memmap_destroy(result);
        return ret;
    }
    ret = clSetKernelArg(result->kernel, 1, sizeof(cl_mem), (void *) &result->taps_obj);
    printf("clSetKernelArg: %d\n", ret);
    if (ret != 0) {
        fir_filter_memmap_destroy(result);
        return ret;
    }
    ret = clSetKernelArg(result->kernel, 2, sizeof(cl_uint), &result->taps_len);
    printf("clSetKernelArg: %d\n", ret);
    if (ret != 0) {
        fir_filter_memmap_destroy(result);
        return ret;
    }
    ret = clSetKernelArg(result->kernel, 3, sizeof(cl_mem), (void *) &result->output_obj);
    printf("clSetKernelArg: %d\n", ret);
    if (ret != 0) {
        fir_filter_memmap_destroy(result);
        return ret;
    }
    printf("decimation: %d\n", result->decimation);
    ret = clSetKernelArg(result->kernel, 4, sizeof(cl_uint), &result->decimation);
    printf("clSetKernelArg: %d\n", ret);
    if (ret != 0) {
        fir_filter_memmap_destroy(result);
        return ret;
    }

    *filter = result;
    return 0;
}

int fir_filter_memmap_process(const float complex *input, size_t input_len, float complex **output, size_t *output_len, fir_filter_memmap *filter) {
    if (input_len > filter->max_input_buffer_length) {
        fprintf(stderr, "<3>requested buffer %zu is more than max: %zu\n", input_len, filter->max_input_buffer_length);
        *output = NULL;
        *output_len = 0;
        return -1;
    }
    if (input_len == 0) {
        *output = NULL;
        *output_len = 0;
        return 0;
    }

    cl_int ret;
    filter->working_buffer = clEnqueueMapBuffer(filter->command_queue, filter->input_obj, CL_TRUE, CL_MAP_WRITE, 0, filter->working_len_total, 0, NULL, NULL, &ret);
    if (ret != CL_SUCCESS) {
        printf("clEnqueueMapBuffer data: %d\n", ret);
        return ret;
    }
    memcpy(filter->working_buffer + filter->history_offset, input, sizeof(float complex) * input_len);
    ret = clEnqueueUnmapMemObject(filter->command_queue, filter->input_obj, filter->working_buffer, 0, NULL, NULL);
    if (ret != CL_SUCCESS) {
        printf("clEnqueueUnmapMemObject data: %d\n", ret);
        return ret;
    }
    size_t working_len = filter->history_offset + input_len;

    if (working_len < filter->original_taps_len) {
        filter->history_offset = working_len;
        *output = NULL;
        *output_len = 0;
        return 0;
    }
//    size_t current_index = 0;
    // input might not have enough data to produce output sample
//    printf("input items: %zu\n", input_len);
    size_t result_len = (size_t) ceilf((float) input_len / filter->decimation);
//    printf("total output items: %zu\n", result_len);
//    printf("taps len: %zu\n", filter->taps_len);
    size_t output_len_rounded = (size_t) ceilf((float) result_len / 12);
//    printf("total output items: %zu\n", output_len_rounded);
    size_t local_item_size = 12;
    size_t work_items = 12;
//    current_index = result_len * filter->decimation;
//    size_t local_item_size = 2; // Process in groups of 64
//        printf("number of items per single qpu: %d\n", output_len_obj);
    ret = clSetKernelArg(filter->kernel, 5, sizeof(cl_uint), &output_len_rounded);
//        printf("clSetKernelArg: %d\n", ret);
    if (ret != 0) {
        return ret;
    }

//    printf("writing: %zu\n", working_len);
//    ret = clEnqueueWriteBuffer(filter->command_queue, filter->input_obj, CL_TRUE, 0,
//                               working_len * sizeof(float complex), filter->working_buffer, 0, NULL, NULL);
//    if (ret != CL_SUCCESS) {
//        printf("clEnqueueWriteBuffer data: %d\n", ret);
//        return ret;
//    }
    ret = clEnqueueNDRangeKernel(filter->command_queue, filter->kernel, 1, NULL,
                                 &work_items, &local_item_size, 0, NULL, NULL);
    if (ret != 0) {
        printf("clEnqueueNDRangeKernel: %d\n", ret);
        return ret;
    }
    ret = clEnqueueReadBuffer(filter->command_queue, filter->output_obj, CL_TRUE, 0,
                              result_len * sizeof(float complex), filter->output, 0, NULL, NULL);
    if (ret != 0) {
        printf("clEnqueueReadBuffer: %d\n", ret);
        return ret;
    }
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

    // preserve history for the next execution
//    filter->history_offset = working_len - current_index;

//    printf("history offset: %zu\n", filter->history_offset);
    filter->history_offset = filter->original_taps_len - 1 - (result_len * filter->decimation - input_len);
    filter->working_buffer = clEnqueueMapBuffer(filter->command_queue, filter->input_obj, CL_TRUE, CL_MAP_WRITE, 0, filter->working_len_total, 0, NULL, NULL, &ret);
    if (ret != CL_SUCCESS) {
        printf("clEnqueueMapBuffer data: %d\n", ret);
        return ret;
    }
    memmove(filter->working_buffer, filter->working_buffer + (working_len - filter->history_offset), sizeof(float complex) * filter->history_offset);
    ret = clEnqueueUnmapMemObject(filter->command_queue, filter->input_obj, filter->working_buffer, 0, NULL, NULL);
    if (ret != CL_SUCCESS) {
        printf("clEnqueueUnmapMemObject data: %d\n", ret);
        return ret;
    }

    *output = filter->output;
    *output_len = result_len;
    return 0;
}

void fir_filter_memmap_destroy(fir_filter_memmap *filter) {
    if (filter == NULL) {
        return;
    }
    if (filter->taps != NULL) {
        free(filter->taps);
    }
    if (filter->original_taps != NULL) {
        free(filter->original_taps);
    }
    if (filter->output != NULL) {
        free(filter->output);
    }
//    if (filter->working_buffer != NULL) {
//        free(filter->working_buffer);
//    }
    if (filter->command_queue != NULL) {
        cl_int ret = clFlush(filter->command_queue);
        if (ret != 0) {
            fprintf(stderr, "unable to flush queue: %d\n", ret);
        }
        ret = clFinish(filter->command_queue);
        if (ret != 0) {
            fprintf(stderr, "unable to finish queue: %d\n", ret);
        }
    }
    if (filter->kernel != NULL) {
        cl_int ret = clReleaseKernel(filter->kernel);
        if (ret != 0) {
            fprintf(stderr, "unable to release kernel: %d\n", ret);
        }
    }
    if (filter->program != NULL) {
        cl_int ret = clReleaseProgram(filter->program);
        if (ret != 0) {
            fprintf(stderr, "unable to release program: %d\n", ret);
        }
    }
    if (filter->input_obj != NULL) {
        cl_int ret = clReleaseMemObject(filter->input_obj);
        if (ret != 0) {
            fprintf(stderr, "unable to release memory: %d\n", ret);
        }
    }
    if (filter->taps_obj != NULL) {
        cl_int ret = clReleaseMemObject(filter->taps_obj);
        if (ret != 0) {
            fprintf(stderr, "unable to release memory: %d\n", ret);
        }
    }
    if (filter->output_obj != NULL) {
        cl_int ret = clReleaseMemObject(filter->output_obj);
        if (ret != 0) {
            fprintf(stderr, "unable to release memory: %d\n", ret);
        }
    }
    if (filter->command_queue != NULL) {
        cl_int ret = clReleaseCommandQueue(filter->command_queue);
        if (ret != 0) {
            fprintf(stderr, "unable to release command queue: %d\n", ret);
        }
    }
    if (filter->context != NULL) {
        cl_int ret = clReleaseContext(filter->context);
        if (ret != 0) {
            fprintf(stderr, "unable to release context: %d\n", ret);
        }
    }
    free(filter);
}
