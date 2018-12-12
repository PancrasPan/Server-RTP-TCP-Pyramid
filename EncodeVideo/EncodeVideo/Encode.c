/*#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
#include <libavutil/imgutils.h>
#include <libavformat/avformat.h>
 
#define FRAME_PER_SECOND 30     //帧率
#define PICTURE_WIDTH 2560		//图片宽度
#define PICTURE_HEIGHT 1600     //图片高度
#define DEBUG    //调试选项,写文件操作

static void encode(AVCodecContext *enc_ctx, AVFrame *frame, AVPacket *pkt, FILE *outfile)
{
	int ret;
	//将一帧图像送入编码器
#ifdef DEBUG
	if (frame)
		printf("Send frame %3"PRId64"\n", frame->pts);
#endif
	ret = avcodec_send_frame(enc_ctx, frame);
	if (ret < 0) {
		fprintf(stderr, "Error sending a frame for encoding\n");
		exit(1);
	}

	while (ret >= 0) {
		ret = avcodec_receive_packet(enc_ctx, pkt);
		if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) //现在还不能取出编码后的码流
			return;
		else if (ret < 0) {    //编码错误
			fprintf(stderr, "Error during encoding\n");
			exit(1);
		}
#ifdef DEBUG
		printf("Write packet %3"PRId64" (size=%5d)\n", pkt->pts, pkt->size);
		fwrite(pkt->data, 1, pkt->size, outfile);
#endif // DEBUG	
		av_packet_unref(pkt);
	}
}

void x264_encodeVideo(char *filename, uint8_t *pic_inbuff, AVPacket *pkt,int end)
{
	const AVCodec *codec;
	AVCodecContext *c = NULL;
	int i, ret, pic_size;
	FILE *fin, *fout;
	AVFrame *frame;
	uint8_t endcode[] = { 0, 0, 1, 0xb7 };

	//查找需要的编码器
	codec = avcodec_find_encoder_by_name("libx264");
	if (!codec) {
		fprintf(stderr, "Codec libx264 not found\n");
		exit(1);
	}
	//按照编码器的默认值分配AVCodecContext内容以及默认值
	c = avcodec_alloc_context3(codec);
	if (!c) {
		fprintf(stderr, "Could not allocate video codec context\n");
		exit(1);
	}

	c->bit_rate = 10000000;  //编码比特率
	c->width = PICTURE_WIDTH;    //输入分辨率
	c->height = PICTURE_HEIGHT;
	c->time_base = (AVRational) { 1, FRAME_PER_SECOND };
	c->framerate = (AVRational) { FRAME_PER_SECOND, 1 };
	c->gop_size = 8;  //每GOP帧做一次帧内预测
	c->max_b_frames = 3;
	c->pix_fmt = AV_PIX_FMT_YUV420P;
	c->qmin = 10;
	c->qmax = 51;

	if (codec->id == AV_CODEC_ID_H264) {
		av_opt_set(c->priv_data, "preset", "medium", 0);
		//av_opt_set(c->priv_data, "tune", "zerolatency", 0);
	}

	//打开编码器
	ret = avcodec_open2(c, codec, NULL);
	if (ret < 0) {
		fprintf(stderr, "Could not open codec: %s\n", av_err2str(ret));
		exit(1);
	}

	//申请帧操作方法,通过format/width/height三个参数来确定帧缓存大小
	frame = av_frame_alloc();
	if (!frame) {
		fprintf(stderr, "Could not allocate video frame\n");
		exit(1);
	}
	frame->format = c->pix_fmt;
	frame->width = c->width;
	frame->height = c->height;
	ret = av_frame_get_buffer(frame, 32);     //后一个参数需要根据电脑CPU型号选择对齐方式，0表示自适应
	if (ret < 0) {
		fprintf(stderr, "Could not allocate the video frame data\n");
		exit(1);
	}

	//编码后负载数据包的大小和分配
	pkt = av_packet_alloc();
	if (!pkt)
		exit(1);
	ret = av_image_get_buffer_size(c->pix_fmt, c->width, c->height, 32);  //32位对齐方式
	pic_inbuff = (uint8_t *)malloc(ret * sizeof(uint8_t));
	av_new_packet(pkt, ret);   //data_buffer,需要包括每帧头的大小
	pic_size = PICTURE_HEIGHT*PICTURE_WIDTH;
#ifdef DEBUG
	fopen_s(&fout, "out.h264", "wb");
	if (!fout) {
		fprintf(stderr, "Could not open out.h264\n");
		exit(1);
	}
#endif // DEBUG
	frame->data[0] = pic_inbuff;  //Y分量
	frame->data[1] = pic_inbuff + pic_size; //U分量
	frame->data[2] = pic_inbuff + pic_size * 5 / 4; //V分量
	frame->pts = i;   //编码一帧输出一帧
	encode(c, frame, pkt, fout);


	if (end) {  //传输结束信号，需要进行编码器结束操作
		 flush the encoder 
		encode(c, NULL, pkt, fout);
		add sequence end code to have a real H264 file 
		fwrite(endcode, 1, sizeof(endcode), fout);
#ifdef DEBUG
		fclose(fout);
#endif // DEBUG
		avcodec_free_context(&c);
		av_frame_free(&frame);
		av_packet_free(&pkt);
	}
}
int main(int argc, char **argv)
{
	char *filename;
	if (argc < 2) {
		fprintf(stderr, "Usage: %s <output file>\n", argv[0]);
		exit(0);
	}
	filename = argv[1];
	AVPacket *pkt=NULL;
	x264_encodeVideo(filename, pkt);
	return 0;
}*/