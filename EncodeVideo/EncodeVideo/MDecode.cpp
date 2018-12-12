/*#include "MDecode.h"
using namespace std;

class MDecode
{
protected:
	AVCodecParserContext *parser;
	AVCodecContext *context;
	AVFrame *frame;
	AVPacket *packet;
	uint8_t *pic_inbuff;
	uint8_t *pic_outbuff;
	ifstream fin_index,fin_video;
	ofstream fout_yuv;
public:
	MDecode();  //构造函数初始化
	int decoder_init(AVCodecID CodeID);
	int decoder_decode(uint32_t seq_number, uint32_t data_size);
	void decoder_flush(void);
	void decoder_destroy(void);
	void write_decode_frame();
	void read_file_init(string name);
	void read_file_by_index(int gop_num, short angle);
	void read_file_destroy(string name);
};

MDecode::MDecode()
{
}

//初始化读文件操作，按照文件名
void MDecode::read_file_init(string name, short resolution)
{
	
}

//将解码后的视频帧写入帧缓存
void MDecode::write_decode_frame()
{
	memcpy(pic_outbuff, frame->data[0], PIC_SIZE * sizeof(uint8_t));
	memcpy(pic_outbuff + PIC_SIZE, frame->data[1], PIC_SIZE / 4 * sizeof(uint8_t));
	memcpy(pic_outbuff + 5 * PIC_SIZE / 4, frame->data[0], PIC_SIZE / 4 * sizeof(uint8_t));
	fout_yuv.write((char*)&pic_outbuff, PIC_SIZE * 3 / 2);
}

//解码器初始化
int MDecode::decoder_init(AVCodecID CodeID) //AV_CODEC_ID_H264
{

	const AVCodec *codec;
	//申请读入数据的buffer，计算buffer_size(假设buffer_size=4096)
	pic_inbuff = (uint8_t*)malloc(BUFFER_SIZE * sizeof(uint8_t));

	//查找x264的解码器
	codec = avcodec_find_decoder(CodeID);
	if (!codec) {
		fprintf(stderr, "Codec not found\n");
		exit(1);
	}
	context = avcodec_alloc_context3(codec);
	if (!context) {
		fprintf(stderr, "Could not allocate video codec context\n");
		exit(1);
	}
	//按照编码器类型初始化解码器参数
	parser = av_parser_init(codec->id);
	if (!parser) {
		fprintf(stderr, "parser not found\n");
		exit(1);
	}

	//对msmpeg4以及mpeg4类型的编码器必须在此处给出宽度和高度，因为编码bit流里不包含该类文件
	//在avcodec_alloc_context3之后使用，初始化AVCodeContext以便使用AVCodeContext
	if (avcodec_open2(context, codec, NULL) < 0) {
		fprintf(stderr, "Could not open codec\n");
		exit(1);
	}

	//申请编码后的数据包以及帧
	packet = av_packet_alloc();
	if (!packet)
		exit(1);
	frame = av_frame_alloc();
	if (!frame) {
		fprintf(stderr, "Could not allocate video frame\n");
		exit(1);
	}
	return 1;
}

//清空解码缓冲区
void MDecode::decoder_flush(void)
{
	int ret;
	ret = avcodec_send_packet(context, NULL);
	if (ret < 0) {
		fprintf(stderr, "Error sending a packet for decoding\n");
		exit(1);
	}

	while (ret >= 0) {
		ret = avcodec_receive_frame(context, frame);
		if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)  //解码还未完成
			return;
		else if (ret < 0) {
			fprintf(stderr, "Error during decoding\n"); //解码出错
			exit(1);
		}
		
//解码器分配的图片，不用释放该buffer 
#ifdef FRAME_INFO
		printf("saving frame %3d,width=%d,height=%d\n", context->frame_number, context->width, context->height);
		fflush(stdout);
#endif 
		write_decode_frame();
	}
}

//释放解码器占用内存
void MDecode::decoder_destroy()
{
	av_parser_close(parser);
	avcodec_free_context(&context);
	av_frame_free(&frame);
	av_packet_free(&packet);
}

//输入一帧进行解码
int MDecode::decoder_decode(uint32_t seq_number, uint32_t data_size)
{
	int ret;
	uint8_t *data;
	data = pic_inbuff;
	if (data_size <= 0) {  //如果解码数据量为不是正整数
		return -1;
	}
	//使用parser将读入的数据解析为帧
	while (data_size> 0) {
		ret = av_parser_parse2(parser, context, &packet->data, &packet->size, data, data_size, AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0);
		if (ret < 0) {
			fprintf(stderr, "Error while parsing\n");
			exit(1);
		}
		data += ret;   //数据指针后移
		data_size -= ret; //当前使用的数据量

		printf_s("ret=%d,data_size=%d,pkt->size=%d\n", ret, data_size, packet->size);
		if (packet->size) { //一帧分割完成,解码一帧
			ret = avcodec_send_packet(context, packet);
			if (ret < 0) {
				fprintf(stderr, "Error sending a packet for decoding\n");
				exit(1);
			}
			while (ret >= 0) {
				ret = avcodec_receive_frame(context, frame);
				if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)  //解码还未完成
					break;
				else if (ret < 0) {
					fprintf(stderr, "Error during decoding\n"); //解码出错
					exit(1);
				}
#ifdef FRAME_INFO
				printf("saving frame %3d,width=%d,height=%d\n", context->frame_number, frame->width, frame->height);
				fflush(stdout);
#endif 
				write_decode_frame();
			}
		}
	}
	return 0;
}

int main()
{
	AVCodecParserContext *parser;
	AVCodecContext *c = NULL;
	AVFrame *frame;
	uint8_t *inbuf;
	size_t   data_size;
	AVPacket *pkt;

#ifdef DEBUG
	FILE *fin;
	const char *filename;
	filename = "out.h264";
	fopen_s(&fin, filename, "rb");
	if (!fin) {
		fprintf(stderr, "Could not open %s\n", filename);
		exit(1);
	}

	FILE *fout;
	fopen_s(&fout, "out.yuv", "wb");
	if (!fout) {
		fprintf(stderr, "Could not open out.yuv\n");
		exit(1);
	}
#endif // DEBUG

	x264_decoder_Init(&parser, &c, &frame, &pkt, &inbuf);
	while (!feof(fin)) {
		//读入编码后的二进制码流
		data_size = fread(inbuf, 1, BUFFER_SIZE, fin);
		if (!data_size) {
			break;
		}
		//使用parser将读入的数据解析为帧
		x264_decodeVideo(&parser, &c, &pkt, frame, inbuf, data_size, fout);
	}
	x264_decoder_Destroy(&c, &parser, &frame, &pkt, fout);
	return 0;
}
*/