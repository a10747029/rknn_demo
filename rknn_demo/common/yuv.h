#ifndef _YUV_H_
#define _YUV_H_

#ifdef __cplusplus
extern "C"{
#endif

#define READ_MAX (1024)
typedef unsigned char  uInt8;
typedef unsigned short uInt16;
typedef unsigned int uInt32;
typedef char Int8;
typedef short Int16;
typedef int Int32;
typedef enum
{
        TYPE_YUV422I_UYVY,
        TYPE_YUV422I_YUYV,
        TYPE_YUV420SP_NV12,
        TYPE_YUV420SP_NV21,
        TYPE_YUV420P_YV12,
        TYPE_YUV420P_I420,
        TYPE_YUV422P,
        TYPE_YUV444I,
        TYPE_YUV444P,
}enYuvType;

typedef enum
{
        YUV_GREEN,
        YUV_RED,
        YUV_BLUE,
        YUV_PURPLE,
        YUV_DARK_GREEN,
        YUV_YELLOW,
        YUV_LIGHT_BLUE,
        YUV_LIGHT_PURPLE,
        YUV_DARK_BLACK,
        YUV_GRAY,
        YUV_WHITE,
        YUV_COLOR_MAX,
}enYuvColorIdx;

typedef struct
{
        uInt8 Y;
        uInt8 U;
        uInt8 V;
}stYuvColor;

typedef struct
{
        uInt16 x;
        uInt16 y;
}stPoint;

typedef struct
{
        stPoint startPoint;
        stPoint endPoint;
        uInt16 lineWidth;
        enYuvColorIdx clrIdx;
}stDrawLineInfo,stDrawRectInfo;

typedef struct
{
        enYuvType yuvType;
        uInt8 *pYuvBuff;
        uInt16 width;
        uInt16 height;
}stYuvBuffInfo;

static stYuvColor s_color_table[YUV_COLOR_MAX] = {
        {0x00, 0x00, 0x00}, // green
        {0x00, 0x00, 0xff}, // red
        {0x00, 0xff, 0x00},     // blue
        {0x00, 0xff, 0xff},     // purple
        {0xff, 0x00, 0x00}, // dark green
        {0xff, 0x00, 0xff}, // yellow
        {0xff, 0xff, 0x00}, // light blue
        {0xff, 0xff, 0xff}, // light purple
        {0x00, 0x80, 0x80}, // dark black
        {0x80, 0x80, 0x80}, // gray
        {0xff, 0x80, 0x80}, // white
};

//line1,(0,100)(100,480)
#define LINE_1_B_X 0
#define LINE_1_B_Y 100
#define LINE_1_E_X 100
#define LINE_1_E_Y 480
#define LINE_1_COLOR YUV_PURPLE

//line2,(0,100)(200,480)
#define LINE_2_B_X 0
#define LINE_2_B_Y 100
#define LINE_2_E_X 200
#define LINE_2_E_Y 480
#define LINE_2_COLOR YUV_BLUE

//line3,(0,100)(300,480)
#define LINE_3_B_X 0
#define LINE_3_B_Y 100
#define LINE_3_E_X 300
#define LINE_3_E_Y 480
#define LINE_3_COLOR YUV_GREEN

#define DEFAULT_RECT_COLOR YUV_YELLOW

#define LINE1(X)  (((LINE_1_E_Y - LINE_1_B_Y)/(LINE_1_E_X - LINE_1_B_X))*(X- LINE_1_B_X) + LINE_1_B_Y)
#define LINE2(X)  (((LINE_2_E_Y - LINE_2_B_Y)/(LINE_2_E_X - LINE_2_B_X))*(X- LINE_2_B_X) + LINE_2_B_Y)
#define LINE3(X)  (((LINE_3_E_Y - LINE_3_B_Y)/(LINE_3_E_X - LINE_3_B_X))*(X- LINE_3_B_X) + LINE_3_B_Y)
void YUV420toYUV444(int width, int height, unsigned char* src, unsigned char* dst);
int YUV420toRGB24(int width, int height, unsigned char* src, unsigned char* dst);

int YUV420toRGB24_RGA(unsigned int src_fmt, unsigned char* src_buf,
                      int src_w, int src_h,
                      unsigned int dst_fmt, int dst_fd,
                      int dst_w, int dst_h);
int YUV420toRGB24_RGA_update(unsigned char* dest_buf,unsigned int src_fmt, unsigned char* src_buf,
                      int src_w, int src_h,
                      unsigned int dst_fmt, int dst_fd,
                      int dst_w, int dst_h);
void yuv_drawline(stYuvBuffInfo *pYuvBuffInfo, stDrawLineInfo *pDrawLineInfo);
void draw_rect(stYuvBuffInfo* yuvBuffInfo,stDrawRectInfo *pDrawRectInfo);

#ifdef __cplusplus
}
#endif

#endif
