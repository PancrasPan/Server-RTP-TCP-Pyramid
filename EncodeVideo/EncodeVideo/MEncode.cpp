#include "MEncode.h"
#include "RtpSender.h"
#include <windows.h>
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
	ofstream fout_video,fout_index;
	short frame_num;  //��ǰд���˶���֡
	uint32_t index;
	int offset1, offset2;
public:
	MEncode();
	int encoder_init();
	void encoder_destroy(void);
	void encoder_encode(int seq_number);
	void encoder_flush(void);
	void write_video_by_resolution_init(string video_name,short angle);
	void write_video_index_init(string video_name);   //ÿ����Ƶ��������30���ӽǵ�
	void write_file_destroy(string video_name);
	void write_encode_frame(char * str, uint32_t size);
	void write_video_end(short angle);
	void encoder_excute(string video_name);
};
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
		cerr<<"Could not open "<<video_name<<"_"<<angle<<".yuv"<<endl;
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
		//uint8_t endcode[] = { 0x00, 0x00, 0x00, 0x01, 0x61};
		//fout_video.write((char*)endcode, 5);
		//index += 5;
//#ifdef DEBUG
		std::cout << "дһ��GOP��" << index << endl;
//#endif // DEBUG	
		fout_index.write((char*)&index, 4);	//д���±�
		frame_num = 0;
	}
	fout_video.write(str, size);
	index += size;
	++frame_num;
}

//����ǰ���������Ƶд���ļ�
//void MEncode::write_video_end(short angle)
//{
//	fout_index.write((char*)&index, 4);
//
//#ifdef DEBUG
//	ftest << "дһ��GOP��" << index << endl;
//	cout << "д��һ���ļ����±�" << fout_index.tellp() << endl;
//#endif // DEBUG
//
//	uint8_t endcode[] = { 0, 0, 1, 0xb7 };
//	fout_video.write((char*)endcode,4);
//	if (angle < ANG_NUM - 1) {
//		fin_video.close();	//�ر��ļ�
//		fout_video.close();
//		int pos = fout_index.tellp();   //��λ��ǰָ��
//		fout_index.seekp(ELEM_SIZE*(angle + 1));
//		fout_index.write((char*)&pos, 4);	//�޸ĳ�ʼ�ֱ����±�ָ��			
//		fout_index.seekp(pos);   //�ƶ������һ���ֽ�
//		index = 0;
//		fout_index.write((char*)&index, 4); //��һ���±���Ҫ�ٴ�0��ʼ�����ֱַ�����Ƶ�������ļ����洢
//		frame_num = 0;
//	}
//}
void MEncode::write_video_end(short angle)
{
	fout_index.write((char*)&index, 4);

#ifdef DEBUG
	ftest << "д��һ���ļ���" << index << "  λ��=" << fout_index.tellp() << endl;
#endif // DEBUG

	//uint8_t endcode[] = { 0, 0, 1, 0xb7 };
	//fout_video.write((char*)endcode, 4);
	fin_video.close();	//�ر��ļ�
	fout_video.close();
	if (angle < ANG_NUM - 1) {
		int pos = fout_index.tellp();   //��λ��ǰָ��
		fout_index.seekp(ELEM_SIZE*(angle + 1));
		fout_index.write((char*)&pos, 4);	//�޸ĳ�ʼ�ֱ����±�ָ��
#ifdef DEBUG
		ftest << "�޸ĵ�" << angle + 1 << "�ļ�����ʼ�±�Ϊ��" << pos << endl;
#endif // DEBUG
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
	codec = avcodec_find_encoder(AV_CODEC_ID_H264); //������Ҫ�ı�����
	//codec = avcodec_find_encoder_by_name("libx264");
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
	//encoder->mb
	if (codec->id == AV_CODEC_ID_H264) {
		av_opt_set(encoder->priv_data, "preset", "medium", 0);
		//av_opt_set(encoder->priv_data, "tune", "psnr", 0);
		av_opt_set(encoder->priv_data, "tune", "zerolatency", 0);
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
	ret = av_frame_get_buffer(frame,32);     //!!!!!!!!!!!!!!!!!!�������������Y���/2��������������
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
		//sender.SendPacket((void*)packet->data, packet->size);
		av_packet_unref(packet);
		//RTPTime::Wait(RTPTime(0, 30000));
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
		printf("Flush Write packet %3" PRId64" (size=%5d)\n", packet->pts, packet->size);
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
			cout << seq_number<<"="<<fin_video.gcount() << endl;
			encoder_encode(seq_number++);
		}
		encoder_flush();		//ÿ��flush���ͷű���ṹ
		write_video_end(i);
	}	
	encoder_destroy();
	write_file_destroy(video_name);
}

class TServer
{
protected:
	WSAData wsaData;
	SOCKET listenfd,connfd;  //��������Ǹ�����ͻ�����������connfd���鼴��
public:
	//���䵽�ͻ��˺���
	TServer::TServer();
	SOCKET get_connfd();
	int set_non_block(SOCKET socket);//����socket������
	int send_non_block(SOCKET socket, char *buffer, int length, int flags);//��������������
	int recv_non_block(SOCKET socket, char *buffer, int length, int flags);//��������������
	int server_transfer_init();//��ʼ��socket����
	void server_transfer_destroy();//���ٷ���˴���socket															   
};

//���캯��
TServer::TServer()
{
	
}
SOCKET TServer::get_connfd()
{
	return connfd;
}

/*�������׽��ֵ�recv*/
int TServer::recv_non_block(SOCKET socket, char *buffer, int length, int flags)
{
	int recv_len, ret_val, sel;
	struct timeval tm;

	for (recv_len = 0; recv_len < length;)
	{
		/*�ö���*/
		fd_set read_fd;
		FD_ZERO(&read_fd);
		FD_SET(socket, &read_fd);
		//�ȴ�1s���ղ����ͷ���
		tm.tv_sec = 10;    //1��
		tm.tv_usec = 1;    //1u��			  
		sel = select(socket + 1, &read_fd, NULL, NULL, &tm);  /*����select*/
		if (sel < 0) {   //����ʧ��
			printf("select socket error: (errno: %d)\n", WSAGetLastError());
			return -1;
		}
		else if (sel == 0) {//��ʱ���ؽ��յ�����
			printf("Recv timout!: (errno: %d)\n", WSAGetLastError());
			return recv_len;
		}
		else {
			if (FD_ISSET(socket, &read_fd)) { //���������д
				ret_val = recv(socket, buffer + recv_len, length - recv_len, flags);
				if (ret_val < 0) {
					printf("recv error\n");
					return -2;
				}
				else if (ret_val == 0) {
					printf("connection closed\n");
					return 0;
				}
				else
					recv_len += ret_val;
			}
		}
	}
	return recv_len;
}

/*�������׽��ֵ�send*/
int TServer::send_non_block(SOCKET socket, char *buffer, int length, int flags)
{
	int send_len, ret_val, sel;
	struct timeval tm;

	for (send_len = 0; send_len < length;)
	{
		/*��д��*/
		fd_set write_fd;
		FD_ZERO(&write_fd);
		FD_SET(socket, &write_fd);
		//�����������ϴ󣬵�1s���Ͳ��˾ͷ���
		tm.tv_sec = 5;
		tm.tv_usec = 1;

		sel = select(socket + 1, NULL, &write_fd, NULL, &tm);/*����select*/
		if (sel <0) {   //����ʧ��
			printf("select socket error: (errno: %d)\n", WSAGetLastError());
			return -1;
		}
		else if (sel == 0) {
			printf("Send time out(1s)\n");
			return send_len;
		}
		else {
			if (FD_ISSET(socket, &write_fd)) { //���������д
				ret_val = send(socket, buffer + send_len, length - send_len, flags);
				if (ret_val < 0) {
					printf("send socket error: (errno: %d)\n", WSAGetLastError());
					return -2;
				}
				else if (ret_val == 0) {
					printf("connection closed\n");
					return 0;
				}
				else
					send_len += ret_val;

			}
		}
	}
	return send_len;
}


/*�����׽���Ϊ������ģʽ*/
int TServer::set_non_block(SOCKET socket)
{
	/*��ʶ����0���������ģʽ*/
	int ret;
	unsigned long flag = 1;
	ret = ioctlsocket(socket, FIONBIO, &flag);
	if (ret)
		printf("set nonblock error: (errno: %d)\n", WSAGetLastError());
	return ret;
}

//��ʼ��socket����
int TServer::server_transfer_init()
{
	int clientaddr_len, sel;
	int flag = 1;
	SOCKADDR_IN servaddr, clientaddr;
	//�ö���
	fd_set read_fd;
	timeval tt;

	if (WSAStartup(MAKEWORD(2, 2), &wsaData)) {  //�汾2
		printf("Fail to initialize windows socket!\n");
		return -1;
	}
	//����һ���׽���
	if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		printf("create socket error: (errno: %d)\n", WSAGetLastError());
		return -1;
	}
	//�����׽���Ϊ������ģʽ
	if (set_non_block(listenfd)) {
		closesocket(listenfd);
		return -1;
	}
	//��ʼ���׽���*/
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(PORT_NUMBER);
	servaddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);

	/*��������˿��ظ���*/
	if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (const char*)&flag, sizeof(int)) == -1) {
		printf("set socket option error: (errno: %d)\n", WSAGetLastError());
		closesocket(listenfd);
		return -1;
	}

	/*�󶨶˿�*/
	if (bind(listenfd, (const struct sockaddr*)&servaddr, sizeof(servaddr)) == -1) {
		printf("bind socket error: (errno: %d)\n", WSAGetLastError());
		closesocket(listenfd);
		return -1;
	}

	/*�����˿�*/
	if (listen(listenfd, MAX_LIS_NUM) == -1) {
		printf("listen socket error: (errno: %d)\n", WSAGetLastError());
		closesocket(listenfd);
		return -1;
	}

//#ifdef DEBUG
	printf("======waiting for client's request======\n");
//#endif

	//���ܶ˿�����
	clientaddr_len = sizeof(SOCKADDR_IN);

	//����select�ȴ�accept
	FD_ZERO(&read_fd);
	FD_SET(listenfd, &read_fd);
	tt.tv_sec = 1000;    //50�볬ʱ,�ȴ��ͻ�������
	tt.tv_usec = 1;    //1u��

	sel = select(listenfd + 1, &read_fd, NULL, NULL, &tt);
	if (sel <= 0) {   //����ʧ��
		printf("select socket error: (errno: %d)\n", WSAGetLastError());
		return -1;
	}

	if ((connfd = accept(listenfd, (struct sockaddr*)&clientaddr, &clientaddr_len)) == -1) {
		printf("accept socket error: (errno: %d)\n", WSAGetLastError());
		closesocket(listenfd);
		return -1;
	}
//#ifdef DEBUG
	char buffer[512];
	inet_ntop(AF_INET, &clientaddr.sin_addr, buffer, 512);
	printf("client IP:%s, port:%d, connected\n", buffer, ntohs(clientaddr.sin_port));
//#endif
	return 0;
}

//���ٷ���˴���socket
void TServer::server_transfer_destroy()
{
	/* �ȴ��ͻ��˹ر����� */
	char buffer[256];
	int ret;
	while (1)
	{
		ret = recv(connfd, buffer, 256, 0);
		if (ret <= 0) {
			closesocket(connfd);
			printf("Client close\n");
			break;
		}
		Sleep(1000);  //˯��1s�ȴ��ر�
	}
	closesocket(listenfd);
	WSACleanup();
}


class RGop
{
protected:
	uint32_t base,start,end;  //����ַ
	ifstream f_index, f_video[ANG_NUM]; //ֱ�ӿ�30���ӽ��ļ�
	short old_angle;  //�ɽǶ���Ϣ���Ƿ�ҪŲ�������ļ�
	TServer server;
	SOCKET connfd;
	char *pic_buff;
	int  len;
public:
	RGop();  //���캯��
	~RGop();
	//��ȡGop����
	void read_video_init(string video_name);
	void read_index_by_gopAngle(uint32_t gop_num, short angle);
	void read_video_by_index(short angle);
	void receive_video_excute(string video_name);
	void RTP_execute(RtpSender &sender, string video_name);
	void read_rtp_video(short angle);


};

RGop::RGop()
{
	pic_buff = new char[BUFF_SIZE + 6];
	old_angle = -1;
}
RGop::~RGop()
{
	delete pic_buff;
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
		f_index.seekg(ios::beg + angle*ELEM_SIZE);  //����angle���±꣬����Index�ļ���ʼ�ĵڼ����ֽ�
		f_index.read((char*)&base, ELEM_SIZE);
		f_index.seekg(base);
	}
	f_index.seekg(base + gop_num*ELEM_SIZE);	//�ƶ�����ǰ�����ڵ�angle������ʼ���±�
	f_index.read((char*)&start, ELEM_SIZE);
	f_index.read((char*)&end, ELEM_SIZE);

	len = end - start;
	f_video[angle].seekg(start);
#ifdef DEBUG
	printf("start:%d end:%d\n", start, end);
#endif
}

//������ʼ�ļ��±꿪ʼ��ȡ�ļ�
void RGop::read_video_by_index(short angle)
{
	uint32_t pos = start;//ÿ�ζ�1024�ֽ�
	int num = 0;
	memcpy(pic_buff, (char*)&len, 4);
	memcpy(pic_buff + 4, (char*)&angle, 2);
	num = server.send_non_block(connfd, pic_buff,6, 0);//
	cout << "send head len=" << num << endl;


	f_video[angle].seekg(start);
	for (pos = start; (end - pos) >= BUFF_SIZE; pos += BUFF_SIZE)
	{
		f_video[angle].read(pic_buff, BUFF_SIZE);
		if (f_video[angle].gcount() != BUFF_SIZE)
		{
			cout << "READ ERROR!" << endl;
			break;
		}
		//len = BUFF_SIZE;
		//len = htons(len);
		//memcpy(pic_buff, (char*)&len, 2);
		//memcpy(pic_buff+2, (char*)&angle, 4);
		num=server.send_non_block(connfd, pic_buff, BUFF_SIZE, 0);//
		//cout <<"send_len="<<num << endl;

#ifdef DEBUG
		read_video.write(pic_buff+2, BUFF_SIZE);
#endif // DEBUG	
	}
	start = end - pos;
	f_video[angle].read(pic_buff, start);

	//len = start;
	//len = htons(len);
	//memcpy(pic_buff, (char*)&len, 2);
	//memcpy(pic_buff, (char*)&angle, 4);
	num=server.send_non_block(connfd, pic_buff, start, 0);//
	cout << "send_len=" << num << endl;
	if (f_video[angle].gcount() != start)
	{
		cout << "READ ERROR!" << endl;
		return;
	}
#ifdef DEBUG
	read_video.write(pic_buff+2, start);
#endif // DEBUG	
}


//���տͻ��˴���������Ϣ���Ҷ�ȡ��Ҫ���ļ��鴫��@!!!!!!����ģ��
void RGop::receive_video_excute(string video_name)
{
	read_video_init(video_name);
	while(1)
	{
		//��ʼ��socket
		if (server.server_transfer_init() == -1) {
			printf("Socket error!\n");
			exit(1);
		}
		short angle;
		uint32_t gop_num = 0;
		connfd = server.get_connfd();
		while (1) {
			int ret = server.recv_non_block(connfd, pic_buff, 2, 0);
			if (ret == 2)
			{
				memcpy(&angle, pic_buff, 2); //2�ֽڵ�angle
											 //memcpy(&gop_num, pic_buff + 2, 4); //4�ֽڵ�gop_num
											 //gop_num = ntohl(gop_num);
											 //angle = ntohs(angle);
				cout << "gop_num=" << gop_num << " angle=" << angle << endl;

				if (angle == -1 || gop_num == -1) {  //��ʱ��������
					break;
				}
				read_index_by_gopAngle(gop_num, angle);
				read_video_by_index(angle);
				gop_num = (gop_num + 1) % 30;
			}
			else
				break;

		}
		server.server_transfer_destroy();
	}
	return;
}
void RGop::read_rtp_video(short angle)
{
	len = end - start;
	printf("%d %d %d\n",end,start, len);
	f_video[angle].read(pic_buff, len);
}
void RGop::RTP_execute(RtpSender &sender, string video_name)
{

	_LARGE_INTEGER time_start;    /*��ʼʱ��*/
	_LARGE_INTEGER time_over;        /*����ʱ��*/
	double dqFreq;                /*��ʱ��Ƶ��*/
	LARGE_INTEGER f;            /*��ʱ��Ƶ��*/
	QueryPerformanceFrequency(&f);
	dqFreq = (double)f.QuadPart;
	
	
	read_video_init(video_name);
	// check incoming packets
	int viewpointIndex = 0;
	int i = 0;
	while (true)
	{
		sender.BeginDataAccess();
		if (sender.GotoFirstSourceWithData())
		{
			do
			{
				RTPPacket *pack;
				RTPSourceData *srcdat;

				srcdat = sender.GetCurrentSourceInfo();

				while ((pack = sender.GetNextPacket()) != NULL)
				{
					if (pack->GetPayloadType() == VIEWPOINT)
					{
						memcpy(&viewpointIndex, pack->GetPayloadData(), pack->GetPayloadLength());
						printf("viewpoint index:%d\n", viewpointIndex);
						if (viewpointIndex < ANG_NUM)
						{
							read_index_by_gopAngle(i, viewpointIndex);
							sender.SendViewPoint(viewpointIndex);
							QueryPerformanceCounter(&time_start);
							sender.SendPacket(f_video[viewpointIndex], len);
							QueryPerformanceCounter(&time_over);
							cout << ((time_over.QuadPart - time_start.QuadPart) / dqFreq) << endl;//��λΪ�룬����Ϊ1000 000/��cpu��Ƶ��΢��
							
							//RTPTime::Wait(RTPTime(0, 300000));
							i = (++i) % 30;

						}
						
					}
					sender.DeletePacket(pack);
				}
			} while (sender.GotoNextSourceWithData());
		}
		sender.EndDataAccess();
	}
	/*while (1)
	{
		
		sender.dataMutex.Lock();
		if (sender.connected)
		{
			viewpointIndex = sender.viewpointIndex;
			sender.dataMutex.Unlock();

			read_index_by_gopAngle(i, viewpointIndex);
			sender.SendViewPoint(viewpointIndex);
			sender.SendPacket(f_video[viewpointIndex], len);
			i = (++i) % 30;
		}
		else
		{
			sender.dataMutex.Unlock();
			RTPTime::Wait(RTPTime(0, 300000));
		}
		
	}*/
}
int main()
{
#ifndef ENCODE

	RGop gop;

#ifdef RTP


#ifdef RTP_SOCKETTYPE_WINSOCK
	WSADATA dat;
	WSAStartup(MAKEWORD(2, 2), &dat);
#endif // RTP_SOCKETTYPE_WINSOCK
	RtpSender sender;
	sender.SetParams(PORT_NUMBER);

	gop.RTP_execute(sender,"1280-1280");
	RTPTime::Wait(RTPTime(1000, 0));

#else
	gop.receive_video_excute("1280-1280");
#endif


#else

	MEncode encoder;
	encoder.encoder_excute("1280-1280");

#endif
	return 0;
}


