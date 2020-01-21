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


/*
//line1,(0,100)(50,300)
#define LINE_1_B_X  43.0
#define LINE_1_B_Y  178.0
#define LINE_1_E_X  65.0
#define LINE_1_E_Y  300.0
#define LINE_1_COLOR    YUV_PURPLE

//line2,(0,100)(100,300)
#define LINE_2_B_X  53.0
#define LINE_2_B_Y  178.0
#define LINE_2_E_X  105.0
#define LINE_2_E_Y  300.0
#define LINE_2_COLOR    YUV_BLUE

//line3,(0,100)(150,300)
#define LINE_3_B_X  59.0
#define LINE_3_B_Y  178.0
#define LINE_3_E_X  147.0
#define LINE_3_E_Y  300.0
#define LINE_3_COLOR    YUV_GREEN
*/
//line1,(0,100)(50,300)
#define LINE_1_B_X  0.0
#define LINE_1_B_Y  100.0
#define LINE_1_E_X  50.0
#define LINE_1_E_Y  300.0
#define LINE_1_COLOR    YUV_PURPLE

//line2,(0,100)(100,300)
#define LINE_2_B_X  0.0
#define LINE_2_B_Y  100.0
#define LINE_2_E_X  100.0
#define LINE_2_E_Y  300.0
#define LINE_2_COLOR    YUV_BLUE

//line3,(0,100)(150,300)
#define LINE_3_B_X  0.0
#define LINE_3_B_Y  100.0
#define LINE_3_E_X  150.0
#define LINE_3_E_Y  300.0
#define LINE_3_COLOR    YUV_GREEN

#define LINE1(X) (int)(((LINE_1_E_Y - LINE_1_B_Y)/(LINE_1_E_X - LINE_1_B_X))*(X- LINE_1_B_X) + LINE_1_B_Y)
#define LINE2(X) (int)(((LINE_2_E_Y - LINE_2_B_Y)/(LINE_2_E_X - LINE_2_B_X))*(X- LINE_2_B_X) + LINE_2_B_Y)
#define LINE3(X) (int)(((LINE_3_E_Y - LINE_3_B_Y)/(LINE_3_E_X - LINE_3_B_X))*(X- LINE_3_B_X) + LINE_3_B_Y)




int run_ssd(char* src);
int ssd_init();
unsigned char AlertJudge(int x0,int y0,int x1,int y1);
#endif
