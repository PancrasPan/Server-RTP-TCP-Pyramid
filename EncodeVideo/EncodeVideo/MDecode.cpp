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
	MDecode();  //���캯����ʼ��
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

//��ʼ�����ļ������������ļ���
void MDecode::read_file_init(string name, short resolution)
{
	
}

//����������Ƶ֡д��֡����
void MDecode::write_decode_frame()
{
	memcpy(pic_outbuff, frame->data[0], PIC_SIZE * sizeof(uint8_t));
	memcpy(pic_outbuff + PIC_SIZE, frame->data[1], PIC_SIZE / 4 * sizeof(uint8_t));
	memcpy(pic_outbuff + 5 * PIC_SIZE / 4, frame->data[0], PIC_SIZE / 4 * sizeof(uint8_t));
	fout_yuv.write((char*)&pic_outbuff, PIC_SIZE * 3 / 2);
}

//��������ʼ��
int MDecode::decoder_init(AVCodecID CodeID) //AV_CODEC_ID_H264
{

	const AVCodec *codec;
	//����������ݵ�buffer������buffer_size(����buffer_size=4096)
	pic_inbuff = (uint8_t*)malloc(BUFFER_SIZE * sizeof(uint8_t));

	//����x264�Ľ�����
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
	//���ձ��������ͳ�ʼ������������
	parser = av_parser_init(codec->id);
	if (!parser) {
		fprintf(stderr, "parser not found\n");
		exit(1);
	}

	//��msmpeg4�Լ�mpeg4���͵ı����������ڴ˴�������Ⱥ͸߶ȣ���Ϊ����bit���ﲻ���������ļ�
	//��avcodec_alloc_context3֮��ʹ�ã���ʼ��AVCodeContext�Ա�ʹ��AVCodeContext
	if (avcodec_open2(context, codec, NULL) < 0) {
		fprintf(stderr, "Could not open codec\n");
		exit(1);
	}

	//������������ݰ��Լ�֡
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

//��ս��뻺����
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
		if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)  //���뻹δ���
			return;
		else if (ret < 0) {
			fprintf(stderr, "Error during decoding\n"); //�������
			exit(1);
		}
		
//�����������ͼƬ�������ͷŸ�buffer 
#ifdef FRAME_INFO
		printf("saving frame %3d,width=%d,height=%d\n", context->frame_number, context->width, context->height);
		fflush(stdout);
#endif 
		write_decode_frame();
	}
}

//�ͷŽ�����ռ���ڴ�
void MDecode::decoder_destroy()
{
	av_parser_close(parser);
	avcodec_free_context(&context);
	av_frame_free(&frame);
	av_packet_free(&packet);
}

//����һ֡���н���
int MDecode::decoder_decode(uint32_t seq_number, uint32_t data_size)
{
	int ret;
	uint8_t *data;
	data = pic_inbuff;
	if (data_size <= 0) {  //�������������Ϊ����������
		return -1;
	}
	//ʹ��parser����������ݽ���Ϊ֡
	while (data_size> 0) {
		ret = av_parser_parse2(parser, context, &packet->data, &packet->size, data, data_size, AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0);
		if (ret < 0) {
			fprintf(stderr, "Error while parsing\n");
			exit(1);
		}
		data += ret;   //����ָ�����
		data_size -= ret; //��ǰʹ�õ�������

		printf_s("ret=%d,data_size=%d,pkt->size=%d\n", ret, data_size, packet->size);
		if (packet->size) { //һ֡�ָ����,����һ֡
			ret = avcodec_send_packet(context, packet);
			if (ret < 0) {
				fprintf(stderr, "Error sending a packet for decoding\n");
				exit(1);
			}
			while (ret >= 0) {
				ret = avcodec_receive_frame(context, frame);
				if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)  //���뻹δ���
					break;
				else if (ret < 0) {
					fprintf(stderr, "Error during decoding\n"); //�������
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
		//��������Ķ���������
		data_size = fread(inbuf, 1, BUFFER_SIZE, fin);
		if (!data_size) {
			break;
		}
		//ʹ��parser����������ݽ���Ϊ֡
		x264_decodeVideo(&parser, &c, &pkt, frame, inbuf, data_size, fout);
	}
	x264_decoder_Destroy(&c, &parser, &frame, &pkt, fout);
	return 0;
}
*/