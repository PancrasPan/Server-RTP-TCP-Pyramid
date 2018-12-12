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

/*调用windows socket的库函数*/
#pragma comment(lib,"ws2_32.lib")

#define GOP_SIZE 10   //gop的大小
#define FRAME_PER_SECOND 30     //帧率
#define PICTURE_WIDTH 1280		//图片宽度
#define PICTURE_HEIGHT 1280     //图片高度
#define BIT_RATE 4*1000*1000   //多少Mbps
#define PATH "C:\\Users\\cic-vr\\Documents\\Visual Studio 2015\\Projects\\Cloud VR Server"
#define BUFF_SIZE 1024
#define ANG_NUM 30 //多少个视角
#define ELEM_SIZE 4 //定义Index文件中每个元素的bit位宽
#define PORT_NUMBER 6666	//定义端口号
#define MAX_LIS_NUM 10  //定义最大监听队列长度
//#define DEBUG
#define FRAME_INFO
//#define ENCODE
//#define RTP
													   																