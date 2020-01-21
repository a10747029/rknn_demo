/***************************************************************************
 *   Copyright (C) 2012 by Tobias Müller                                   *
 *   Tobias_Mueller@twam.info                                              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

/**
	Convert from YUV420 format to YUV444.

	\param width width of image
	\param height height of image
	\param src source
	\param dst destination
*/
#include <stdlib.h>
#include "yuv.h"
#include <rga/RgaApi.h>

void YUV420toYUV444(int width, int height, unsigned char* src, unsigned char* dst) {
	int line, column;
	unsigned char *py, *pu, *pv;
	unsigned char *tmp = dst;

	// In this format each four bytes is two pixels. Each four bytes is two Y's, a Cb and a Cr.
	// Each Y goes to one of the pixels, and the Cb and Cr belong to both pixels.
	unsigned char *base_py = src;
	unsigned char *base_pu = src+(height*width);
	unsigned char *base_pv = src+(height*width)+(height*width)/4;

	for (line = 0; line < height; ++line) {
		for (column = 0; column < width; ++column) {
			py = base_py+(line*width)+column;
			pu = base_pu+(line/2*width/2)+column/2;
			pv = base_pv+(line/2*width/2)+column/2;

			*tmp++ = *py;
			*tmp++ = *pu;
			*tmp++ = *pv;
		}
	}
}

int YUV420toRGB24(int width, int height, unsigned char* src, unsigned char* dst) {
    if (width < 1 || height < 1 || src == NULL || dst == NULL)
        return -1;
    const long len = width * height;
    unsigned char* yData = src;
    unsigned char* vData = &yData[len];
    unsigned char* uData = &vData[len >> 2];

    int bgr[3];
    int yIdx,uIdx,vIdx,idx;
    for (int i = 0;i < height;i++){
        for (int j = 0;j < width;j++){
            yIdx = i * width + j;
            vIdx = (i/2) * (width/2) + (j/2);
            uIdx = vIdx;
  /*  YUV420 转 BGR24
            bgr[0] = (int)(yData[yIdx] + 1.732446 * (uData[vIdx] - 128)); // b分量
            bgr[1] = (int)(yData[yIdx] - 0.698001 * (uData[uIdx] - 128) - 0.703125 * (vData[vIdx] - 128));// g分量
            bgr[2] = (int)(yData[yIdx] + 1.370705 * (vData[uIdx] - 128)); // r分量
 */
 /*  YUV420 转 RGB24 注意如转换格式不对应会导致颜色失真*/
            bgr[0] = (int)(yData[yIdx] + 1.370705 * (vData[uIdx] - 128)); // r分量
            bgr[1] = (int)(yData[yIdx] - 0.698001 * (uData[uIdx] - 128) - 0.703125 * (vData[vIdx] - 128));// g分量
            bgr[2] = (int)(yData[yIdx] + 1.732446 * (uData[vIdx] - 128)); // b分量

            for (int k = 0;k < 3;k++){
                idx = (i * width + j) * 3 + k;
                if(bgr[k] >= 0 && bgr[k] <= 255)
                    dst[idx] = bgr[k];
                else
                    dst[idx] = (bgr[k] < 0)?0:255;
            }
        }
    }
    return 0;
}

int YUV420toRGB24_RGA(unsigned int src_fmt, unsigned char* src_buf,
                      int src_w, int src_h,
                      unsigned int dst_fmt, int dst_fd,
                      int dst_w, int dst_h)
{
    int ret;
    rga_info_t src;
    rga_info_t dst;
    int Format;

    memset(&src, 0, sizeof(rga_info_t));
    src.fd = -1; //rga_src_fd;
    src.virAddr = src_buf;
    src.mmuFlag = 1;

    memset(&dst, 0, sizeof(rga_info_t));
    dst.fd = dst_fd;
    dst.mmuFlag = 1;


    rga_set_rect(&src.rect, 0, 0, src_w, src_h, src_w, src_h, src_fmt);
    rga_set_rect(&dst.rect, 0, 0, dst_w, dst_h, dst_w, dst_h, dst_fmt);
    ret = c_RkRgaBlit(&src, &dst, NULL);
    if (ret)
        printf("c_RkRgaBlit0 error : %s\n", strerror(errno));
    return ret;
}
int YUV420toRGB24_RGA_update(unsigned char* dest_buf,unsigned int src_fmt, unsigned char* src_buf,
                      int src_w, int src_h,
                      unsigned int dst_fmt, int dst_fd,
                      int dst_w, int dst_h)
{
    int ret;
    rga_info_t src;
    rga_info_t dst;
    int Format;

    memset(&src, 0, sizeof(rga_info_t));
    src.fd = -1; //rga_src_fd;
    src.virAddr = src_buf;
    src.mmuFlag = 1;
    memset(&dst, 0, sizeof(rga_info_t));
    dst.fd = -1;
    dst.virAddr = dest_buf;
    dst.mmuFlag = 1;


    rga_set_rect(&src.rect, 0, 0, src_w, src_h, src_w, src_h, src_fmt);
    rga_set_rect(&dst.rect, 0, 0, dst_w, dst_h, dst_w, dst_h, dst_fmt);
    ret = c_RkRgaBlit(&src, &dst, NULL);
    if (ret)
        printf("c_RkRgaBlit0 error : %s\n", strerror(errno));
    return ret;
}


static void yuv_setdata(
	uInt8* YBuff,
	uInt8* UVBuff,
	enYuvType yuvType,
	uInt16 width,
	uInt16 height,
	stPoint draw_point,
	enYuvColorIdx clrIdx)
{
	switch(yuvType)
	{
		case TYPE_YUV422I_UYVY:
		case TYPE_YUV422I_YUYV:
		{
			/*
				UYVY UYVY UYVY UYVY
			*/
			uInt32 tmp = draw_point.y * width * 2;
			uInt32 y_offset = 0, u_offset = 0, v_offset = 0;
			if(yuvType == TYPE_YUV422I_UYVY) {
				u_offset = tmp + draw_point.x / 2 * 4;
				v_offset = u_offset + 2;
				y_offset = u_offset + 1;
			}
			else {
				y_offset = tmp + draw_point.x / 2 * 4;
				u_offset = y_offset + 1;
				v_offset = u_offset + 2;
			}
			YBuff[y_offset] = s_color_table[clrIdx].Y;
			YBuff[y_offset + 2] = s_color_table[clrIdx].Y;
			YBuff[u_offset] = s_color_table[clrIdx].U;
			YBuff[v_offset] = s_color_table[clrIdx].V;
		}break;
		case TYPE_YUV420SP_NV12:
		case TYPE_YUV420SP_NV21:
		{
			/*
				YY YY
				YY YY
				UV UV
			*/
			uInt32 y_offset = draw_point.y * width + draw_point.x;
			uInt32 u_offset = 0, v_offset = 0;
			YBuff[y_offset] = s_color_table[clrIdx].Y;

			if(yuvType == TYPE_YUV420SP_NV12) {
				u_offset = (draw_point.y / 2) * width + draw_point.x / 2 * 2;
				v_offset = u_offset + 1;
			}
			else {
				v_offset = (draw_point.y / 2) * width + draw_point.x / 2 * 2;
				u_offset = v_offset + 1;
			}
			UVBuff[u_offset] = s_color_table[clrIdx].U;
			UVBuff[v_offset] = s_color_table[clrIdx].V;
			//printf("[%d, %d]: y_offset = %d, u_offset = %d, v_offset = %d\n",
			//	draw_point.x, draw_point.y, y_offset, u_offset, v_offset);
		}break;
		case TYPE_YUV420P_YV12:
                case TYPE_YUV420P_I420:
                {
                        /*
                                YY YY
                                YY YY
                                UU VV
                        */
                        uInt32 y_offset = draw_point.y * width + draw_point.x;
                        uInt32 u_offset = 0, v_offset = 0;
			uInt32 plane_size = (width * height / 4);
                        YBuff[y_offset] = s_color_table[clrIdx].Y;


                        if(yuvType == TYPE_YUV420P_YV12) {
                        	u_offset = ((draw_point.y / 2) * width) / 2 + draw_point.x / 2;
                        	v_offset = plane_size + u_offset;
                        }
                        else {
                        	v_offset = ((draw_point.y / 2) * width) / 2 + draw_point.x / 2;;
                        	u_offset = plane_size + v_offset;
                        }
                        UVBuff[u_offset] = s_color_table[clrIdx].U;
                        UVBuff[v_offset] = s_color_table[clrIdx].V;
                        //printf("[%d, %d]: y_offset = %d, u_offset = %d, v_offset = %d\n",
                        //      draw_point.x, draw_point.y, y_offset, u_offset, v_offset);
                }break;
		case TYPE_YUV444P:
		{
			/*
				YYYYYYYY
				UUUUUUUU
				VVVVVVVV
			*/
			uInt32 y_offset = 0, u_offset = 0, v_offset = 0;
			uInt32 plane_size = width * height;
			y_offset = draw_point.y * width + draw_point.x;
			u_offset = y_offset;
			v_offset = plane_size + u_offset;
			YBuff[y_offset] = s_color_table[clrIdx].Y;
			UVBuff[u_offset] = s_color_table[clrIdx].U;
			UVBuff[v_offset] = s_color_table[clrIdx].V;
		}break;
		case TYPE_YUV444I:
		{
			/*
				YUV YUV YUV YUV YUV YUV YUV YUV
			*/
			uInt32 y_offset = 0, u_offset = 0, v_offset = 0;
			y_offset = draw_point.y * width * 3 + draw_point.x * 3;
			u_offset = y_offset + 1;
			v_offset = u_offset + 1;
			YBuff[y_offset] = s_color_table[clrIdx].Y;
			YBuff[u_offset] = s_color_table[clrIdx].U;
			YBuff[v_offset] = s_color_table[clrIdx].V;
		}break;
		case TYPE_YUV422P:
		{
			/*
				YYYYYYYY
				UUUU
				VVVV
			*/
			uInt32 y_offset = 0, u_offset = 0, v_offset = 0;
			uInt32 plane_size = width * height / 2;
			y_offset = draw_point.y * width + draw_point.x;
			u_offset = (draw_point.y / 2) * width + draw_point.x / 2;
			v_offset = plane_size + u_offset;
			YBuff[y_offset] = s_color_table[clrIdx].Y;
			UVBuff[u_offset] = s_color_table[clrIdx].U;
			UVBuff[v_offset] = s_color_table[clrIdx].V;
		}break;
	}
}

void yuv_drawline(stYuvBuffInfo *pYuvBuffInfo, stDrawLineInfo *pDrawLineInfo)
{
	if(!pYuvBuffInfo || !pYuvBuffInfo->pYuvBuff) return;

	uInt8 *YBuff = NULL, *UVBuff = NULL;
	uInt16 x0 = pDrawLineInfo->startPoint.x, y0 = pDrawLineInfo->startPoint.y;
	uInt16 x1 = pDrawLineInfo->endPoint.x, y1 = pDrawLineInfo->endPoint.y;

	if(pDrawLineInfo->lineWidth == 0) pDrawLineInfo->lineWidth = 1;
	x0 = (x0 >= pYuvBuffInfo->width) ? (x0 - pDrawLineInfo->lineWidth) : x0;
	x1 = (x1 >= pYuvBuffInfo->width) ? (x1 - pDrawLineInfo->lineWidth) : x1;
	y0 = (y0 >= pYuvBuffInfo->height) ? (y0 - pDrawLineInfo->lineWidth) : y0;
	y1 = (y1 >= pYuvBuffInfo->height) ? (y1 - pDrawLineInfo->lineWidth) : y1;

	uInt16 dx = (x0 > x1) ? (x0 - x1) : (x1 - x0);
	uInt16 dy = (y0 > y1) ? (y0 - y1) : (y1 - y0);

	Int16 xstep = (x0 < x1) ? 1 : -1;
	Int16 ystep = (y0 < y1) ? 1 : -1;
	Int16 nstep = 0, eps = 0;

	stPoint draw_point;
	draw_point.x = x0;
	draw_point.y = y0;

	switch(pYuvBuffInfo->yuvType)
	{
		case TYPE_YUV422I_UYVY:
		case TYPE_YUV422I_YUYV:
		case TYPE_YUV444I:
		{
			YBuff = pYuvBuffInfo->pYuvBuff;
			UVBuff = NULL;
		}break;
		case TYPE_YUV420SP_NV12:
		case TYPE_YUV420SP_NV21:
		case TYPE_YUV420P_YV12:
		case TYPE_YUV420P_I420:
		case TYPE_YUV444P:
		case TYPE_YUV422P:
		{
			YBuff = pYuvBuffInfo->pYuvBuff;
			UVBuff = pYuvBuffInfo->pYuvBuff + pYuvBuffInfo->width * pYuvBuffInfo->height;
		}break;
		default:
			return;
	}

	// 布雷森汉姆算法画线
	if(dx > dy){
		while(nstep <= dx) {
			yuv_setdata(YBuff, UVBuff, pYuvBuffInfo->yuvType, pYuvBuffInfo->width, pYuvBuffInfo->height, draw_point, pDrawLineInfo->clrIdx);
			eps += dy;
			if( (eps << 1) >= dx ) {
				draw_point.y += ystep;
				eps -= dx;
			}
			draw_point.x += xstep;
			nstep++;
		}
	}else {
		while(nstep <= dy){
			yuv_setdata(YBuff, UVBuff, pYuvBuffInfo->yuvType, pYuvBuffInfo->width, pYuvBuffInfo->height, draw_point, pDrawLineInfo->clrIdx);
			eps += dx;
			if( (eps << 1) >= dy ) {
				draw_point.x += xstep;
				eps -= dy;
			}
			draw_point.y += ystep;
			nstep++;
		}
	}
}

void draw_rect(stYuvBuffInfo* yuvBuffInfo,stDrawRectInfo *pDrawRectInfo)
{
	stDrawLineInfo drawLineInfo;
	drawLineInfo.clrIdx = pDrawRectInfo->clrIdx;
	drawLineInfo.lineWidth = pDrawRectInfo->lineWidth;
	drawLineInfo.startPoint.x = pDrawRectInfo->startPoint.x;
	drawLineInfo.startPoint.y = pDrawRectInfo->startPoint.y;
	drawLineInfo.endPoint.x = pDrawRectInfo->startPoint.x;
	drawLineInfo.endPoint.y = pDrawRectInfo->endPoint.y;
	yuv_drawline(yuvBuffInfo, &drawLineInfo);

	drawLineInfo.startPoint.x = pDrawRectInfo->startPoint.x;
	drawLineInfo.startPoint.y = pDrawRectInfo->endPoint.y;
	drawLineInfo.endPoint.x = pDrawRectInfo->endPoint.x;
	drawLineInfo.endPoint.y = pDrawRectInfo->endPoint.y;
	yuv_drawline(yuvBuffInfo, &drawLineInfo);

	drawLineInfo.startPoint.x = pDrawRectInfo->endPoint.x;
	drawLineInfo.startPoint.y = pDrawRectInfo->endPoint.y;
	drawLineInfo.endPoint.x = pDrawRectInfo->endPoint.x;
	drawLineInfo.endPoint.y = pDrawRectInfo->startPoint.y;
	yuv_drawline(yuvBuffInfo, &drawLineInfo);

	drawLineInfo.startPoint.x = pDrawRectInfo->endPoint.x;
	drawLineInfo.startPoint.y = pDrawRectInfo->startPoint.y;
	drawLineInfo.endPoint.x = pDrawRectInfo->startPoint.x;
	drawLineInfo.endPoint.y = pDrawRectInfo->startPoint.y;
	yuv_drawline(yuvBuffInfo, &drawLineInfo);
}
