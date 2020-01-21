#ifndef _ALGORITHM__H_
#define _ALGORITHM__H_
#include "rknn_api.h"
#define IMG_CHANNEL         3
#define MODEL_INPUT_SIZE    300
#define NUM_RESULTS         1778
#define NUM_CLASS	    2

#define Y_SCALE  10.0f
#define X_SCALE  10.0f
#define H_SCALE  5.0f
#define W_SCALE  5.0f

int run_ssd(char* src);
int ssd_init();
#endif
