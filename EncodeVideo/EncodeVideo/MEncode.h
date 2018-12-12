#pragma once

#include <WinSock2.h>
#include <WS2tcpip.h>
#include <cstdio>
#include <windows.h>
#include <time.h>
#include <cstdlib>
#include <string>
#include <iostream>
#include <fstream>



extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
#include <libavutil/imgutils.h>
#include <libavformat/avformat.h>
}

/*����windows socket�Ŀ⺯��*/
#pragma comment(lib,"ws2_32.lib")

#define GOP_SIZE 10   //gop�Ĵ�С
#define FRAME_PER_SECOND 30     //֡��
#define PICTURE_WIDTH 1280		//ͼƬ���
#define PICTURE_HEIGHT 1280     //ͼƬ�߶�
#define BIT_RATE 4*1000*1000   //����Mbps
#define PATH "C:\\Users\\cic-vr\\Documents\\Visual Studio 2015\\Projects\\Cloud VR Server"
#define BUFF_SIZE 1024
#define ANG_NUM 30 //���ٸ��ӽ�
#define ELEM_SIZE 4 //����Index�ļ���ÿ��Ԫ�ص�bitλ��
#define PORT_NUMBER 6666	//����˿ں�
#define MAX_LIS_NUM 10  //�������������г���
//#define DEBUG
#define FRAME_INFO
//#define ENCODE
//#define RTP
													   																