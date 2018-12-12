#include "MEncode.h"
using namespace std;
FILE *fout;
ofstream fodr;

//初始化操作(编码器，输入帧，输出包，原始一帧yuv图像缓存)
int x264_encoder_Init(AVCodecContext **c,  AVFrame **frame, AVPacket **pkt, uint8_t **pic_inbuff,int *seq_number)
{
	const AVCodec *codec;
	int ret;
	seq_number = 0;
	//查找需要的编码器
	codec = avcodec_find_encoder_by_name("libx264");
	if (!codec) {
		fprintf(stderr, "Codec libx264 not found\n");
		exit(1);
	}
	//按照编码器的默认值分配AVCodecContext内容以及默认值
	*c = avcodec_alloc_context3(codec);
	if (!(*c)) {
		fprintf(stderr, "Could not allocate video codec context\n");
		exit(1);
	}

	(*c)->bit_rate = BIT_RATE;  //编码比特率
	(*c)->width = PICTURE_WIDTH;    //输入分辨率
	(*c)->height = PICTURE_HEIGHT;
	(*c)->time_base = AVRational{ 1, FRAME_PER_SECOND };
	(*c)->framerate = AVRational{ FRAME_PER_SECOND, 1 };
	(*c)->gop_size = 8;  //每GOP帧做一次帧内预测
	(*c)->max_b_frames = 0;  //不使用b帧
	(*c)->pix_fmt = AV_PIX_FMT_YUV420P;
	(*c)->qmin = 10;
	(*c)->qmax = 51;
	(*c)->flags = FF_CMP_PSNR;
	if (codec->id == AV_CODEC_ID_H264) {
		av_opt_set((*c)->priv_data, "preset", "medium", 0);
		av_opt_set((*c)->priv_data, "tune", "psnr", 0);
		//av_opt_set(c->priv_data, "tune", "zerolatency", 0);
	}

	//打开编码器
	ret = avcodec_open2(*c, codec, NULL);
	if (ret < 0) {
		fprintf(stderr, "Could not open codec: %s\n", av_err2str(ret));
		exit(1);
	}

	//申请帧操作方法,通过format/width/height三个参数来确定帧缓存大小
	*frame = av_frame_alloc();
	if (!frame) {
		fprintf(stderr, "Could not allocate video frame\n");
		exit(1);
	}
	(*frame)->format = (*c)->pix_fmt;
	(*frame)->width =(*c)->width;
	(*frame)->height = (*c)->height;
	ret = av_frame_get_buffer(*frame, 32);     //后一个参数需要根据电脑CPU型号选择对齐方式，0表示自适应
	if (ret < 0) {
		fprintf(stderr, "Could not allocate the video frame data\n");
		exit(1);
	}

	ret = av_image_get_buffer_size(AV_PIX_FMT_YUV420P, PICTURE_WIDTH, PICTURE_HEIGHT, 32);  //32位对齐方式
	*pic_inbuff = (uint8_t *)malloc(ret * sizeof(uint8_t)); 
	//编码后负载数据包的大小和分配
	*pkt = av_packet_alloc();
	if (!pkt)
		exit(1);
	av_new_packet(*pkt, ret);   //data_buffer,需要包括每帧头的大小
	return 1;
}

//销毁申请内存
void x264_encoder_Destroy(AVCodecContext **c, AVFrame **frame, AVPacket **pkt,uint8_t **pic_inbuff)
{
	fclose(fout);
	avcodec_free_context(c);
	av_frame_free(frame);
	av_packet_free(pkt);
	free(*pic_inbuff);
}

//从左至右：输出文件指针  输入缓存 原始yuv图像每帧的大小 每帧图像编号
void x264_encodeVideo(AVCodecContext *c, AVFrame *frame, AVPacket *pkt, uint8_t *pic_inbuff,int seq_number)
{	
	int ret;
	frame->data[0] = pic_inbuff;  //Y分量
	frame->data[1] = pic_inbuff + PIC_SIZE; //U分量
	frame->data[2] = pic_inbuff + PIC_SIZE * 5 / 4; //V分量
	frame->pts = seq_number;   //编码一帧输出一帧
	
	//将一帧图像送入编码器
#ifdef FRAME_INFO
	if (frame)
		printf("Send frame %3" PRId64"\n", frame->pts);
#endif
	ret = avcodec_send_frame(c, frame);
	if (ret < 0) {
		fprintf(stderr, "Error sending a frame for encoding\n");
		exit(1);
	}

	while (ret >= 0) {
		ret = avcodec_receive_packet(c, pkt);
		if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) //现在还不能取出编码后的码流
			return;
		else if (ret < 0) {    //编码错误
			fprintf(stderr, "Error during encoding\n");
			exit(1);
		}
#ifdef FRAME_INFO
		printf("Write packet pts->%3" PRId64" dts->%3d" PRId64" (size=%5d)\n", pkt->pts, pkt->dts,pkt->size);
		fodr << "Write packet pts->" << pkt->pts << " dts->" << pkt->dts << " size->" << pkt->size << endl;
#endif
		fwrite(pkt->data, 1, pkt->size, fout);
		av_packet_unref(pkt);
	}
}

//清出缓冲区，把编码器里的内容
void x264_encoder_Flush(AVCodecContext *c, AVPacket *pkt)
{
	int ret;
	//读出编码器缓存
	ret = avcodec_send_frame(c, NULL);
	if (ret < 0) {
		fprintf(stderr, "Error sending a frame for encoding\n");
		exit(1);
	}

	while (ret >= 0) {
		ret = avcodec_receive_packet(c, pkt);
		if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) //现在还不能取出编码后的码流
			return;
		else if (ret < 0) {    //编码错误
			fprintf(stderr, "Error during encoding\n");
			exit(1);
		}
#ifdef FRAME_INFO
		printf("Write packet %3" PRId64" (size=%5d)\n", pkt->pts, pkt->size);
#endif
		fwrite(pkt->data, 1, pkt->size, fout);
		av_packet_unref(pkt);
	}
	// 添加h264文件结束标志0,0,0,0xb7
	uint8_t endcode[] = { 0, 0, 1, 0xb7 };
#ifdef FRAME_INFO
	printf("Write H.264 end symbol!\n");
#endif // FRAME_INFO
	fwrite(endcode, 1, sizeof(endcode), fout);
}

int main()
{
	int seq_number=0;
	AVCodecContext *c = NULL;
	AVFrame *frame = NULL;
	AVPacket *pkt = NULL;
	uint8_t *pic_inbuff = NULL;
	FILE *fin;
	char *filename;
	int ret;
	//注意传值和传址问题，初始化编码器
	x264_encoder_Init(&c, &frame, &pkt, &pic_inbuff, &seq_number);

	filename = "D:\\TestSeq\\YUV\\test4.yuv";
	fopen_s(&fin, filename, "rb");
	if (!fin) {
		fprintf(stderr, "Could not open %s\n", filename);
		exit(1);
	}

	filename = "D:\\TestSeq\\H264\\test4.h264";
	fopen_s(&fout,filename, "wb");
	if (!fout) {
		fprintf(stderr, "Could not open %s\n",filename);
		exit(1);
	}

	filename = "D:\\TestSeq\\H264\\test4_decode_order1.txt";
	fodr.open(filename,ios::out);
	if (!fodr) {
		fprintf(stderr, "Could not open %s\n", filename);
		exit(1);
	}

	while(!feof(fin)) {
		ret = fread(pic_inbuff, 1, PIC_SIZE * 3 / 2, fin);
		if (ret <= 0) {
			fprintf(stderr, "Read NULL file!\n");
			break;
		}
		else if (feof(fin)) {
			break;
		}
		x264_encodeVideo( c, frame, pkt, pic_inbuff, seq_number++);
	}
	x264_encoder_Flush(c, pkt);
	x264_encoder_Destroy(&c, &frame, &pkt, &pic_inbuff);
	return 0;
}


