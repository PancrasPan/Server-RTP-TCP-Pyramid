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

#define GOP_SIZE 10   //gop�Ĵ�С
#define FRAME_PER_SECOND 30     //֡��
#define PICTURE_WIDTH 744		//ͼƬ���
#define PICTURE_HEIGHT 744     //ͼƬ�߶�
#define BIT_RATE 1*1000*1000   //����Mbps
#define PATH "D:\\VR\\TestVideo"


//����ѡ��
#define FRAME_INFO //ÿ֡�����Ϣ

//��ʼ��x264������
int x264_encoder_Init(AVCodecContext **c, AVFrame **frame, AVPacket **pkt, uint8_t **pic_inbuff,int *seq_number);
//����x264������
void x264_encoder_Destroy(AVCodecContext **c, AVFrame **frame, AVPacket **pkt,uint8_t **pic_inbuff);
//�������ң�����ļ�ָ��  ���뻺�� ԭʼyuvͼ��ÿ֡�Ĵ�С ÿ֡ͼ����
void x264_encodeVideo(AVCodecContext *c, AVFrame *frame, AVPacket *pkt, uint8_t *pic_inbuff ,int seq_number);
//������������ѱ������������
void x264_encoder_Flush(AVCodecContext *c, AVPacket *pkt);

