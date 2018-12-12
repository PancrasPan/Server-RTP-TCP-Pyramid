#pragma once

#include <stdio.h>
#include <windows.h>
#include<time.h>
#include <stdlib.h>
#include <string.h>
#include <fstream>
#include<iostream>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
#include <libavutil/imgutils.h>
#include <libavformat/avformat.h>
}

#define GOP_SIZE 10   //gop的大小
#define FRAME_PER_SECOND 30     //帧率
#define PICTURE_WIDTH 744		//图片宽度
#define PICTURE_HEIGHT 744     //图片高度
#define BIT_RATE 1*1000*1000   //多少Mbps
#define PATH "D:\\VR\\TestVideo"


//调试选项
#define FRAME_INFO //每帧输出信息

//初始化x264编码器
int x264_encoder_Init(AVCodecContext **c, AVFrame **frame, AVPacket **pkt, uint8_t **pic_inbuff,int *seq_number);
//销毁x264编码器
void x264_encoder_Destroy(AVCodecContext **c, AVFrame **frame, AVPacket **pkt,uint8_t **pic_inbuff);
//从左至右：输出文件指针  输入缓存 原始yuv图像每帧的大小 每帧图像编号
void x264_encodeVideo(AVCodecContext *c, AVFrame *frame, AVPacket *pkt, uint8_t *pic_inbuff ,int seq_number);
//清出缓冲区，把编码器里的内容
void x264_encoder_Flush(AVCodecContext *c, AVPacket *pkt);

