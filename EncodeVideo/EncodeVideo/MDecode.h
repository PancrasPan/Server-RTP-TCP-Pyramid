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

#define PIC_SIZE 480*480
#define BUFFER_SIZE 4096
#define FRAME_INFO
#define DEBUG