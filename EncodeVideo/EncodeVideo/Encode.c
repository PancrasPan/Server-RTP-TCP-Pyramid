/*#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
#include <libavutil/imgutils.h>
#include <libavformat/avformat.h>
 
#define FRAME_PER_SECOND 30     //֡��
#define PICTURE_WIDTH 2560		//ͼƬ���
#define PICTURE_HEIGHT 1600     //ͼƬ�߶�
#define DEBUG    //����ѡ��,д�ļ�����

static void encode(AVCodecContext *enc_ctx, AVFrame *frame, AVPacket *pkt, FILE *outfile)
{
	int ret;
	//��һ֡ͼ�����������
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
		if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) //���ڻ�����ȡ������������
			return;
		else if (ret < 0) {    //�������
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

	//������Ҫ�ı�����
	codec = avcodec_find_encoder_by_name("libx264");
	if (!codec) {
		fprintf(stderr, "Codec libx264 not found\n");
		exit(1);
	}
	//���ձ�������Ĭ��ֵ����AVCodecContext�����Լ�Ĭ��ֵ
	c = avcodec_alloc_context3(codec);
	if (!c) {
		fprintf(stderr, "Could not allocate video codec context\n");
		exit(1);
	}

	c->bit_rate = 10000000;  //���������
	c->width = PICTURE_WIDTH;    //����ֱ���
	c->height = PICTURE_HEIGHT;
	c->time_base = (AVRational) { 1, FRAME_PER_SECOND };
	c->framerate = (AVRational) { FRAME_PER_SECOND, 1 };
	c->gop_size = 8;  //ÿGOP֡��һ��֡��Ԥ��
	c->max_b_frames = 3;
	c->pix_fmt = AV_PIX_FMT_YUV420P;
	c->qmin = 10;
	c->qmax = 51;

	if (codec->id == AV_CODEC_ID_H264) {
		av_opt_set(c->priv_data, "preset", "medium", 0);
		//av_opt_set(c->priv_data, "tune", "zerolatency", 0);
	}

	//�򿪱�����
	ret = avcodec_open2(c, codec, NULL);
	if (ret < 0) {
		fprintf(stderr, "Could not open codec: %s\n", av_err2str(ret));
		exit(1);
	}

	//����֡��������,ͨ��format/width/height����������ȷ��֡�����С
	frame = av_frame_alloc();
	if (!frame) {
		fprintf(stderr, "Could not allocate video frame\n");
		exit(1);
	}
	frame->format = c->pix_fmt;
	frame->width = c->width;
	frame->height = c->height;
	ret = av_frame_get_buffer(frame, 32);     //��һ��������Ҫ���ݵ���CPU�ͺ�ѡ����뷽ʽ��0��ʾ����Ӧ
	if (ret < 0) {
		fprintf(stderr, "Could not allocate the video frame data\n");
		exit(1);
	}

	//����������ݰ��Ĵ�С�ͷ���
	pkt = av_packet_alloc();
	if (!pkt)
		exit(1);
	ret = av_image_get_buffer_size(c->pix_fmt, c->width, c->height, 32);  //32λ���뷽ʽ
	pic_inbuff = (uint8_t *)malloc(ret * sizeof(uint8_t));
	av_new_packet(pkt, ret);   //data_buffer,��Ҫ����ÿ֡ͷ�Ĵ�С
	pic_size = PICTURE_HEIGHT*PICTURE_WIDTH;
#ifdef DEBUG
	fopen_s(&fout, "out.h264", "wb");
	if (!fout) {
		fprintf(stderr, "Could not open out.h264\n");
		exit(1);
	}
#endif // DEBUG
	frame->data[0] = pic_inbuff;  //Y����
	frame->data[1] = pic_inbuff + pic_size; //U����
	frame->data[2] = pic_inbuff + pic_size * 5 / 4; //V����
	frame->pts = i;   //����һ֡���һ֡
	encode(c, frame, pkt, fout);


	if (end) {  //��������źţ���Ҫ���б�������������
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