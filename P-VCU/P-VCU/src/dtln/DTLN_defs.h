
#include <cstdio>
#include <sstream>
#include <fstream>
#include <iostream>
#include <vector>
#include <string>
#include <assert.h>
#include <stdint.h>
#include <string.h>


#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <dlfcn.h>

#include "c/c_api.h"


#define DTLN_MODEL_A "model1.tflite"
#define DTLN_MODEL_B "model2.tflite"

#define SAMEPLERATE  (16000)
#define DTLN_BLOCK_LEN		(512)
#define DTLN_BLOCK_SHIFT  (256)
#define DTLN_STATE_SIZE  (256)
#define FFT_OUT_SIZE    (DTLN_BLOCK_LEN / 2 + 1)

struct trg_engine {
    float mic_buffer[DTLN_BLOCK_LEN] = { 0 };
    float lpb_buffer[DTLN_BLOCK_LEN] = {0};
    float out_buffer[DTLN_BLOCK_LEN] = { 0 };

    float states_a[DTLN_STATE_SIZE] = { 0 };
    float states_b[DTLN_STATE_SIZE] = { 0 };

    TfLiteTensor* input_details_1[3], * input_details_2[3];
    const TfLiteTensor* output_details_1[2], * output_details_2[2];
    TfLiteInterpreter* interpreter_1, * interpreter_2;
    TfLiteModel* model1, * model2;
};
