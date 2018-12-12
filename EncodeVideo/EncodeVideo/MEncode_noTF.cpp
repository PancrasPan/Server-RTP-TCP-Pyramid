#include "MEncode.h"
using namespace std;

#ifdef DEBUG
ofstream ftest("testinfo.txt", ios::out);
ofstream read_video("g_video.h264", ios::out | ios::binary);
#endif // !DEBUG

class MEncode
{
protected:
	AVCodecContext *encoder;	//���������
	AVFrame *frame;	 //����֡
	AVPacket *packet; //����������
	uint8_t *pic_inbuff; //����һ֡ͼ��
	ifstream fin_video;
	ofstream fout_video, fout_index;
	short frame_num;  //��ǰд���˶���֡
	uint32_t index;
	int offset1, offset2;
public:
	MEncode();
	int encoder_init();
	void encoder_destroy(void);
	void encoder_encode(int seq_number);
	void encoder_flush(void);
	void write_video_by_resolution_init(string video_name, short angle);
	void write_video_index_init(string video_name);   //ÿ����Ƶ��������30���ӽǵ�
	void write_file_destroy(string video_name);
	void write_encode_frame(char * str, uint32_t size);
	void write_video_end(short angle);
	void encoder_excute(string video_name);
};

class RGop
{
protected:
	uint32_t base, start, end;  //����ַ
	ifstream f_index, f_video[30]; //ֱ�ӿ�30���ӽ��ļ�
	short old_angle;  //�ɽǶ���Ϣ���Ƿ�ҪŲ�������ļ�
	char *pic_buff;
public:
	RGop();  //���캯��
	void read_video_init(string video_name);
	void read_index_by_gopAngle(uint32_t gop_num, short angle);
	void read_video_by_index(short angle);
	void receive_video_excute(string video_name, short angle);
};

RGop::RGop()
{
	pic_buff = new char[BUFF_SIZE];
	old_angle = -1;
}

//��ʼ�����ļ�����
void RGop::read_video_init(string video_name)
{
	string path;
	path = string(PATH) + "\\INDEX\\" + video_name + ".index";
	f_index.open(path, ios::in | ios::binary);
	if (!f_index) {
		fprintf(stderr, "Could not open %s.index\n", video_name);
		exit(1);
	}
	for (short i = 0; i < ANG_NUM; i++) {
		path = string(PATH) + "\\H264\\" + video_name + "_" + to_string(i) + ".h264";
		f_video[i].open(path, ios::in | ios::binary);
		if (!f_video[i]) {
			fprintf(stderr, "Could not open %s_%d.h264\n", video_name, i);
			exit(1);
		}
	}
}

//���մ����gop��Ŷ�ȡ�ļ�
void RGop::read_index_by_gopAngle(uint32_t gop_num, short angle)
{
	if (angle != old_angle) {
		cout << "gop_num=" << gop_num << endl;
		f_index.seekg(ios::beg + angle*ELEM_SIZE);  //����angle���±꣬����Index�ļ���ʼ�ĵڼ����ֽ�
		f_index.read((char*)&base, ELEM_SIZE);
		f_index.seekg(base);
	}
	f_index.seekg(base + gop_num*ELEM_SIZE);	//�ƶ�����ǰ�����ڵ�angle������ʼ���±�
	f_index.read((char*)&start, ELEM_SIZE);
	f_index.read((char*)&end, ELEM_SIZE);
}

//������ʼ�ļ��±꿪ʼ��ȡ�ļ�
void RGop::read_video_by_index(short angle)
{
	uint32_t pos = start;//ÿ�ζ�1024�ֽ�
	for (pos = start; (end - pos) >= BUFF_SIZE; pos += BUFF_SIZE)
	{
		f_video[angle].read(pic_buff, BUFF_SIZE);
		if (f_video[angle].gcount() != BUFF_SIZE)
		{
			cout << "READ ERROR!" << endl;
			break;
		}
#ifdef DEBUG
		read_video.write(pic_buff, BUFF_SIZE);
#endif // DEBUG	
	}
	start = end - pos;
	f_video[angle].read(pic_buff, start);
	if (f_video[angle].gcount() != start)
	{
		cout << "READ ERROR!" << endl;
		return;
	}
#ifdef DEBUG
	read_video.write(pic_buff, start);
#endif // DEBUG	
}

//���տͻ��˴���������Ϣ���Ҷ�ȡ��Ҫ���ļ��鴫��@!!!!!!����ģ��
void RGop::receive_video_excute(string video_name, short angle)
{
	read_video_init(video_name);
	for (uint32_t i = 0; i < 70; i++) {
		read_index_by_gopAngle(i, 0);
		read_video_by_index(angle);
	}
	cout << "����һ����Ƶ" << endl;
	return;
}


MEncode::MEncode()  //���캯��
{
	frame_num = 0;
	index = 0;
	offset1 = PICTURE_WIDTH*PICTURE_HEIGHT;
	offset2 = offset1 * 5 / 4;
}

//��ʼ����Ƶ��д����
void MEncode::write_video_by_resolution_init(string video_name, short angle)
{
	string path;
	path = string(PATH) + "\\YUV\\" + video_name + "_" + to_string(angle) + ".yuv";
	fin_video.open(path, ios::in | ios::binary);
	if (!fin_video) {
		fprintf(stderr, "Could not open %s_%d.yuv\n", video_name, angle);
		exit(1);
	}
	path = string(PATH) + "\\H264\\" + video_name + "_" + to_string(angle) + ".h264";
	fout_video.open(path, ios::out | ios::binary);
	if (!fout_video) {
		fprintf(stderr, "Could not open %s_%d.h264\n", video_name, angle);
		exit(1);
	}
}

//��ʼ���±��д����,30���ӽǵ�gop�ֱ���д��һ���ļ���
//��Ѱ������Ƶ�±�Ϊ[index+1]-[index]�����һ�����У�
void MEncode::write_video_index_init(string video_name)
{
	string path;
	path = string(PATH) + "\\INDEX\\" + video_name + ".index";
	fout_index.open(path, ios::out | ios::binary);   //���赱ǰֻ��һ�ֱַ��ʵ���Ƶ
	if (!fout_index) {
		fprintf(stderr, "Could not open %s.index\n", video_name);
		exit(1);
	}
	index = 0;
	uint32_t temp = index + ELEM_SIZE*ANG_NUM;
	fout_index.write((char*)&temp, 4);
	for (int i = 0; i < ANG_NUM; i++) {
		fout_index.write((char*)&index, 4);	//ǰ30*4���ֽڴ�Ų�ͬ�Ƕ���Ƶ����ʼ�±�,frame_num��0��ʼ��� 
	}
}

//д���ļ���������
void MEncode::write_file_destroy(string video_name)
{
	fin_video.close();
	fout_video.close();
	fout_index.close();
}

//����ǰ����֡д���ļ�
void MEncode::write_encode_frame(char * str, uint32_t size)
{
	if (frame_num == GOP_SIZE) {
#ifdef DEBUG
		ftest << "дһ��GOP��" << index << endl;
#endif // DEBUG	
		fout_index.write((char*)&index, 4);	//д���±�
		frame_num = 0;
	}
	fout_video.write(str, size);
	index += size;
	++frame_num;
}

//����ǰ���������Ƶд���ļ�
void MEncode::write_video_end(short angle)
{
	fout_index.write((char*)&index, 4);

#ifdef DEBUG
	cout << "д��һ���ļ����±�" << fout_index.tellp() << endl;
#endif // DEBUG

	uint8_t endcode[] = { 0, 0, 1, 0xb7 };
	fout_video << endcode;
	if (angle < ANG_NUM - 1) {
		fin_video.close();	//�ر��ļ�
		fout_video.close();
		int pos = fout_index.tellp();   //��λ��ǰָ��
		fout_index.seekp(ELEM_SIZE*(angle + 1));
		fout_index.write((char*)&pos, 4);	//�޸ĳ�ʼ�ֱ����±�ָ��			
		fout_index.seekp(pos);   //�ƶ������һ���ֽ�
		index = 0;
		fout_index.write((char*)&index, 4); //��һ���±���Ҫ�ٴ�0��ʼ�����ֱַ�����Ƶ�������ļ����洢
		frame_num = 0;
	}
}

//���캯��(������������֡�������ͼ���)
int MEncode::encoder_init()
{
	int ret;
	const AVCodec *codec;
	//codec = avcodec_find_encoder(AV_CODEC_ID_H264); //������Ҫ�ı�����
	codec = avcodec_find_encoder_by_name("libx264");
	if (!codec) {
		fprintf(stderr, "Codec libx264 not found\n");
		exit(1);
	}

	//���ձ�������Ĭ��ֵ����AVCodecContext�����Լ�Ĭ��ֵ��ʧ�ܷ���NULL
	encoder = avcodec_alloc_context3(codec);
	if (!(encoder)) {
		fprintf(stderr, "Could not allocate video codec context\n");
		exit(1);
	}

	encoder->bit_rate = BIT_RATE;  //���������
	encoder->width = PICTURE_WIDTH;    //����ֱ���
	encoder->height = PICTURE_HEIGHT;
	encoder->time_base = AVRational{ 1, FRAME_PER_SECOND };
	encoder->framerate = AVRational{ FRAME_PER_SECOND, 1 };
	encoder->gop_size = GOP_SIZE;  //ÿGOP֡��һ��֡��Ԥ��
	encoder->max_b_frames = 0;  //��ʹ��b֡
	encoder->pix_fmt = AV_PIX_FMT_YUV420P;
	encoder->qmin = 10;
	encoder->qmax = 51;
	encoder->flags = FF_CMP_PSNR;

	if (codec->id == AV_CODEC_ID_H264) {
		av_opt_set(encoder->priv_data, "preset", "medium", 0);
		//av_opt_set(encoder->priv_data, "tune", "psnr", 0);
		//av_opt_set(c->priv_data, "tune", "zerolatency", 0);
	}

	//�򿪱�����
	ret = avcodec_open2(encoder, codec, NULL);
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
	frame->format = encoder->pix_fmt;
	frame->width = encoder->width;
	frame->height = encoder->height;
	ret = av_frame_get_buffer(frame, 32);     //��һ��������Ҫ���ݵ���CPU�ͺ�ѡ����뷽ʽ��0��ʾ����Ӧ
	if (ret < 0) {
		fprintf(stderr, "Could not allocate the video frame data\n");
		exit(1);
	}

	ret = av_image_get_buffer_size(AV_PIX_FMT_YUV420P, PICTURE_WIDTH, PICTURE_HEIGHT, 32);  //32λ���뷽ʽ
	pic_inbuff = (uint8_t *)malloc(ret * sizeof(uint8_t));
	//����������ݰ��Ĵ�С�ͷ���
	packet = av_packet_alloc();
	if (!packet) {
		fprintf(stderr, "Could not allocate the packet\n");
		exit(1);
	}
	//av_new_packet(packet, ret);   //data_buffer,��Ҫ����ÿ֡ͷ�Ĵ�С
	return 1;
}

//���������ڴ�
void MEncode::encoder_destroy(void)
{
	avcodec_free_context(&encoder);
	av_frame_free(&frame);
	av_packet_free(&packet);
	free(pic_inbuff);
}

//�������ң�����ļ�ָ��  ���뻺�� ԭʼyuvͼ��ÿ֡�Ĵ�С ÿ֡ͼ����
void MEncode::encoder_encode(int seq_number)
{
	int ret;
	frame->data[0] = pic_inbuff;  //Y����
	frame->data[1] = pic_inbuff + offset1; //U����
	frame->data[2] = pic_inbuff + offset2; //V����
	frame->pts = seq_number;   //����һ֡���һ֡

#ifdef FRAME_INFO
	if (frame)
		printf("Send frame %3" PRId64"\n", frame->pts);
#endif
	ret = avcodec_send_frame(encoder, frame);
	if (ret < 0) {
		fprintf(stderr, "Error sending a frame for encoding\n");
		exit(1);
	}

	while (ret >= 0) {  //���ڻ�����ȡ������������,�ȴ����
		ret = avcodec_receive_packet(encoder, packet);
		if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
			return;
		else if (ret < 0) {    //�������
			fprintf(stderr, "Error during encoding\n");
			exit(1);
		}
#ifdef FRAME_INFO
		printf("Write packet pts->%3" PRId64" dts->%3d" PRId64" (size=%5d)\n", packet->pts, packet->dts, packet->size);
#endif
		write_encode_frame((char*)packet->data, packet->size);
		av_packet_unref(packet);
	}

}

//������������ѱ������������
void MEncode::encoder_flush(void)
{
	int ret;
	//��������������
	ret = avcodec_send_frame(encoder, NULL);
	if (ret < 0) {
		fprintf(stderr, "Error sending a frame for encoding\n");
		exit(1);
	}

	while (ret >= 0) {
		ret = avcodec_receive_packet(encoder, packet);
		if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) //���ڻ�����ȡ������������
			return;
		else if (ret < 0) {    //�������
			fprintf(stderr, "Error during encoding\n");
			exit(1);
		}
#ifdef FRAME_INFO
		printf("Write packet %3" PRId64" (size=%5d)\n", packet->pts, packet->size);
#endif
		write_encode_frame((char*)packet->data, packet->size);
		av_packet_unref(packet);
	}
}

//����ִ�к���
void MEncode::encoder_excute(string video_name)
{
	int seq_number;
	write_video_index_init(video_name);

	for (short i = 0; i < ANG_NUM; i++)
	{
		cout << "��ʼд��" << i << "���ļ�" << endl;
		seq_number = 0;
		encoder_init();
		write_video_by_resolution_init(video_name, i);    //Ŀǰֻдһ�ֱַ��ʵ���Ƶ������ѭ��3��
		while (!fin_video.eof()) {
			fin_video.read((char*)pic_inbuff, offset1 * 3 / 2);
			if (fin_video.gcount() <= 0) {
				fprintf(stderr, "Read NULL file!\n");
				break;
			}
			else if (fin_video.eof()) {
				break;
			}
			encoder_encode(seq_number++);
		}
		encoder_flush();		//ÿ��flush���ͷű���ṹ
		write_video_end(i);
	}	encoder_destroy();
	write_file_destroy(video_name);
}

int main()
{
	/*MEncode x264_encoder;
	x264_encoder.encoder_excute( "480x480_2k");*/
	RGop gop;
	gop.receive_video_excute("480x480_2k", 0);

	return 0;
}


