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
	AVCodecContext *encoder;	//编码控制器
	AVFrame *frame;	 //输入帧
	AVPacket *packet; //编码后输出包
	uint8_t *pic_inbuff; //读入一帧图像
	ifstream fin_video;
	ofstream fout_video,fout_index;
	short frame_num;  //当前写入了多少帧
	uint32_t index;
	int offset1, offset2;
public:
	MEncode();
	int encoder_init();
	void encoder_destroy(void);
	void encoder_encode(int seq_number);
	void encoder_flush(void);
	void write_video_by_resolution_init(string video_name,short angle);
	void write_video_index_init(string video_name);   //每个视频的索引是30个视角的
	void write_file_destroy(string video_name);
	void write_encode_frame(char * str, uint32_t size);
	void write_video_end(short angle);
	void encoder_excute(string video_name);
};
MEncode::MEncode()  //构造函数
{
	frame_num = 0;
	index = 0;
	offset1 = PICTURE_WIDTH*PICTURE_HEIGHT;
	offset2 = offset1 * 5 / 4;
}

//初始化视频读写函数
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

//初始化下标读写函数,30个视角的gop分辨率写到一个文件里
//搜寻两个视频下标为[index+1]-[index]（多加一个换行）
void MEncode::write_video_index_init(string video_name)
{
	string path;
	path = string(PATH) + "\\INDEX\\" + video_name + ".index";
	fout_index.open(path, ios::out | ios::binary);   //假设当前只有一种分辨率的视频
	if (!fout_index) {
		fprintf(stderr, "Could not open %s.index\n", video_name);
		exit(1);
	}
	index = 0;
	uint32_t temp = index + ELEM_SIZE*ANG_NUM;
	fout_index.write((char*)&temp, 4);
	for (int i = 0; i < ANG_NUM; i++) {
		fout_index.write((char*)&index, 4);	//前30*4个字节存放不同角度视频的起始下标,frame_num从0开始编号 
	}
}

//写入文件函数销毁
void MEncode::write_file_destroy(string video_name)
{
	fin_video.close();
	fout_video.close();
	fout_index.close();
}

//将当前编码帧写入文件
void MEncode::write_encode_frame(char * str, uint32_t size)
{
	if (frame_num == GOP_SIZE) {
		//uint8_t endcode[] = { 0x00, 0x00, 0x00, 0x01, 0x61};
		//fout_video.write((char*)endcode, 5);
		//index += 5;
//#ifdef DEBUG
		std::cout << "写一个GOP：" << index << endl;
//#endif // DEBUG	
		fout_index.write((char*)&index, 4);	//写入下标
		frame_num = 0;
	}
	fout_video.write(str, size);
	index += size;
	++frame_num;
}

//将当前编码完的视频写入文件
//void MEncode::write_video_end(short angle)
//{
//	fout_index.write((char*)&index, 4);
//
//#ifdef DEBUG
//	ftest << "写一个GOP：" << index << endl;
//	cout << "写完一个文件的下标" << fout_index.tellp() << endl;
//#endif // DEBUG
//
//	uint8_t endcode[] = { 0, 0, 1, 0xb7 };
//	fout_video.write((char*)endcode,4);
//	if (angle < ANG_NUM - 1) {
//		fin_video.close();	//关闭文件
//		fout_video.close();
//		int pos = fout_index.tellp();   //定位当前指针
//		fout_index.seekp(ELEM_SIZE*(angle + 1));
//		fout_index.write((char*)&pos, 4);	//修改初始分辨率下标指针			
//		fout_index.seekp(pos);   //移动到最后一个字节
//		index = 0;
//		fout_index.write((char*)&index, 4); //下一个下标需要再从0开始，三种分辨率视频分三个文件来存储
//		frame_num = 0;
//	}
//}
void MEncode::write_video_end(short angle)
{
	fout_index.write((char*)&index, 4);

#ifdef DEBUG
	ftest << "写完一个文件：" << index << "  位置=" << fout_index.tellp() << endl;
#endif // DEBUG

	//uint8_t endcode[] = { 0, 0, 1, 0xb7 };
	//fout_video.write((char*)endcode, 4);
	fin_video.close();	//关闭文件
	fout_video.close();
	if (angle < ANG_NUM - 1) {
		int pos = fout_index.tellp();   //定位当前指针
		fout_index.seekp(ELEM_SIZE*(angle + 1));
		fout_index.write((char*)&pos, 4);	//修改初始分辨率下标指针
#ifdef DEBUG
		ftest << "修改第" << angle + 1 << "文件的起始下标为：" << pos << endl;
#endif // DEBUG
		fout_index.seekp(pos);   //移动到最后一个字节
		index = 0;
		fout_index.write((char*)&index, 4); //下一个下标需要再从0开始，三种分辨率视频分三个文件来存储
		frame_num = 0;
	}
}
//构造函数(编码器，输入帧，编码后图像包)
int MEncode::encoder_init()
{
	int ret;
	const AVCodec *codec;
	codec = avcodec_find_encoder(AV_CODEC_ID_H264); //查找需要的编码器
	//codec = avcodec_find_encoder_by_name("libx264");
	if (!codec) {
		fprintf(stderr, "Codec libx264 not found\n");
		exit(1);
	}

	//按照编码器的默认值分配AVCodecContext内容以及默认值，失败返回NULL
	encoder = avcodec_alloc_context3(codec);
	if (!(encoder)) {
		fprintf(stderr, "Could not allocate video codec context\n");
		exit(1);
	}

	encoder->bit_rate = BIT_RATE;  //编码比特率
	encoder->width = PICTURE_WIDTH;    //输入分辨率
	encoder->height = PICTURE_HEIGHT;
	encoder->time_base = AVRational{ 1, FRAME_PER_SECOND };
	encoder->framerate = AVRational{ FRAME_PER_SECOND, 1 };
	encoder->gop_size = GOP_SIZE;  //每GOP帧做一次帧内预测
	encoder->max_b_frames = 0;  //不使用b帧
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

	//打开编码器
	ret = avcodec_open2(encoder, codec, NULL);
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
	frame->format = encoder->pix_fmt;
	frame->width = encoder->width;
	frame->height = encoder->height;
	ret = av_frame_get_buffer(frame,32);     //!!!!!!!!!!!!!!!!!!对齐基数，根据Y宽度/2能整除的最大的数
	if (ret < 0) {
		fprintf(stderr, "Could not allocate the video frame data\n");
		exit(1);
	}

	ret = av_image_get_buffer_size(AV_PIX_FMT_YUV420P, PICTURE_WIDTH, PICTURE_HEIGHT, 32);  //32位对齐方式
	pic_inbuff = (uint8_t *)malloc(ret * sizeof(uint8_t));
	//编码后负载数据包的大小和分配
	packet = av_packet_alloc();
	if (!packet) {
		fprintf(stderr, "Could not allocate the packet\n");
		exit(1);
	}
	//av_new_packet(packet, ret);   //data_buffer,需要包括每帧头的大小
	return 1;
}

//销毁申请内存
void MEncode::encoder_destroy(void)
{
	avcodec_free_context(&encoder);
	av_frame_free(&frame);
	av_packet_free(&packet);
	free(pic_inbuff);
}
//从左至右：输出文件指针  输入缓存 原始yuv图像每帧的大小 每帧图像编号

void MEncode::encoder_encode(int seq_number)
{
	int ret;
	frame->data[0] = pic_inbuff;  //Y分量
	frame->data[1] = pic_inbuff + offset1; //U分量
	frame->data[2] = pic_inbuff + offset2; //V分量
	frame->pts = seq_number;   //编码一帧输出一帧

#ifdef FRAME_INFO
	if (frame)
		printf("Send frame %3" PRId64"\n", frame->pts);
#endif
	ret = avcodec_send_frame(encoder, frame);
	if (ret < 0) {
		fprintf(stderr, "Error sending a frame for encoding\n");
		exit(1);
	}

	while (ret >= 0) {  //现在还不能取出编码后的码流,等待完成
		ret = avcodec_receive_packet(encoder, packet);
		if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
			return;
		else if (ret < 0) {    //编码错误
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

//清出缓冲区，把编码器里的内容
void MEncode::encoder_flush(void)
{
	int ret;
	//读出编码器缓存
	ret = avcodec_send_frame(encoder, NULL);
	if (ret < 0) {
		fprintf(stderr, "Error sending a frame for encoding\n");
		exit(1);
	}

	while (ret >= 0) {
		ret = avcodec_receive_packet(encoder, packet);
		if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) //现在还不能取出编码后的码流
			return;
		else if (ret < 0) {    //编码错误
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

//编码执行函数
void MEncode::encoder_excute(string video_name)
{
	int seq_number;
	write_video_index_init(video_name);

	for (short i = 0; i < ANG_NUM; i++)
	{
		cout << "开始写第" << i << "个文件" << endl;
		seq_number = 0;
		encoder_init();
		write_video_by_resolution_init(video_name, i);    //目前只写一种分辨率的视频，否则循环3次
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
		encoder_flush();		//每次flush会释放编码结构
		write_video_end(i);
	}	
	encoder_destroy();
	write_file_destroy(video_name);
}

class TServer
{
protected:
	WSAData wsaData;
	SOCKET listenfd,connfd;  //后续如果是个多个客户端连接则建立connfd数组即可
public:
	//传输到客户端函数
	TServer::TServer();
	SOCKET get_connfd();
	int set_non_block(SOCKET socket);//设置socket非阻塞
	int send_non_block(SOCKET socket, char *buffer, int length, int flags);//非阻塞发送数据
	int recv_non_block(SOCKET socket, char *buffer, int length, int flags);//非阻塞接收数据
	int server_transfer_init();//初始化socket操作
	void server_transfer_destroy();//销毁服务端传输socket															   
};

//构造函数
TServer::TServer()
{
	
}
SOCKET TServer::get_connfd()
{
	return connfd;
}

/*非阻塞套接字的recv*/
int TServer::recv_non_block(SOCKET socket, char *buffer, int length, int flags)
{
	int recv_len, ret_val, sel;
	struct timeval tm;

	for (recv_len = 0; recv_len < length;)
	{
		/*置读集*/
		fd_set read_fd;
		FD_ZERO(&read_fd);
		FD_SET(socket, &read_fd);
		//等待1s接收不到就返回
		tm.tv_sec = 10;    //1秒
		tm.tv_usec = 1;    //1u秒			  
		sel = select(socket + 1, &read_fd, NULL, NULL, &tm);  /*调用select*/
		if (sel < 0) {   //连接失败
			printf("select socket error: (errno: %d)\n", WSAGetLastError());
			return -1;
		}
		else if (sel == 0) {//超时返回接收的数据
			printf("Recv timout!: (errno: %d)\n", WSAGetLastError());
			return recv_len;
		}
		else {
			if (FD_ISSET(socket, &read_fd)) { //如果真正可写
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

/*非阻塞套接字的send*/
int TServer::send_non_block(SOCKET socket, char *buffer, int length, int flags)
{
	int send_len, ret_val, sel;
	struct timeval tm;

	for (send_len = 0; send_len < length;)
	{
		/*置写集*/
		fd_set write_fd;
		FD_ZERO(&write_fd);
		FD_SET(socket, &write_fd);
		//发送数据量较大，等1s发送不了就返回
		tm.tv_sec = 5;
		tm.tv_usec = 1;

		sel = select(socket + 1, NULL, &write_fd, NULL, &tm);/*调用select*/
		if (sel <0) {   //连接失败
			printf("select socket error: (errno: %d)\n", WSAGetLastError());
			return -1;
		}
		else if (sel == 0) {
			printf("Send time out(1s)\n");
			return send_len;
		}
		else {
			if (FD_ISSET(socket, &write_fd)) { //如果真正可写
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


/*设置套接字为非阻塞模式*/
int TServer::set_non_block(SOCKET socket)
{
	/*标识符非0允许非阻塞模式*/
	int ret;
	unsigned long flag = 1;
	ret = ioctlsocket(socket, FIONBIO, &flag);
	if (ret)
		printf("set nonblock error: (errno: %d)\n", WSAGetLastError());
	return ret;
}

//初始化socket操作
int TServer::server_transfer_init()
{
	int clientaddr_len, sel;
	int flag = 1;
	SOCKADDR_IN servaddr, clientaddr;
	//置读集
	fd_set read_fd;
	timeval tt;

	if (WSAStartup(MAKEWORD(2, 2), &wsaData)) {  //版本2
		printf("Fail to initialize windows socket!\n");
		return -1;
	}
	//创建一个套接字
	if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		printf("create socket error: (errno: %d)\n", WSAGetLastError());
		return -1;
	}
	//设置套接字为非阻塞模式
	if (set_non_block(listenfd)) {
		closesocket(listenfd);
		return -1;
	}
	//初始化套接字*/
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(PORT_NUMBER);
	servaddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);

	/*设置允许端口重复绑定*/
	if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (const char*)&flag, sizeof(int)) == -1) {
		printf("set socket option error: (errno: %d)\n", WSAGetLastError());
		closesocket(listenfd);
		return -1;
	}

	/*绑定端口*/
	if (bind(listenfd, (const struct sockaddr*)&servaddr, sizeof(servaddr)) == -1) {
		printf("bind socket error: (errno: %d)\n", WSAGetLastError());
		closesocket(listenfd);
		return -1;
	}

	/*监听端口*/
	if (listen(listenfd, MAX_LIS_NUM) == -1) {
		printf("listen socket error: (errno: %d)\n", WSAGetLastError());
		closesocket(listenfd);
		return -1;
	}

//#ifdef DEBUG
	printf("======waiting for client's request======\n");
//#endif

	//接受端口连接
	clientaddr_len = sizeof(SOCKADDR_IN);

	//调用select等待accept
	FD_ZERO(&read_fd);
	FD_SET(listenfd, &read_fd);
	tt.tv_sec = 1000;    //50秒超时,等待客户端连接
	tt.tv_usec = 1;    //1u秒

	sel = select(listenfd + 1, &read_fd, NULL, NULL, &tt);
	if (sel <= 0) {   //连接失败
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

//销毁服务端传输socket
void TServer::server_transfer_destroy()
{
	/* 等待客户端关闭连接 */
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
		Sleep(1000);  //睡眠1s等待关闭
	}
	closesocket(listenfd);
	WSACleanup();
}


class RGop
{
protected:
	uint32_t base,start,end;  //基地址
	ifstream f_index, f_video[ANG_NUM]; //直接开30个视角文件
	short old_angle;  //旧角度信息，是否要挪动索引文件
	TServer server;
	SOCKET connfd;
	char *pic_buff;
	int  len;
public:
	RGop();  //构造函数
	~RGop();
	//读取Gop函数
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
//初始化读文件函数
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

//按照传输的gop编号读取文件
void RGop::read_index_by_gopAngle(uint32_t gop_num, short angle)
{
	if (angle != old_angle) {
		f_index.seekg(ios::beg + angle*ELEM_SIZE);  //读到angle的下标，即在Index文件开始的第几个字节
		f_index.read((char*)&base, ELEM_SIZE);
		f_index.seekg(base);
	}
	f_index.seekg(base + gop_num*ELEM_SIZE);	//移动到当前的所在的angle索引起始块下标
	f_index.read((char*)&start, ELEM_SIZE);
	f_index.read((char*)&end, ELEM_SIZE);

	len = end - start;
	f_video[angle].seekg(start);
#ifdef DEBUG
	printf("start:%d end:%d\n", start, end);
#endif
}

//按照起始文件下标开始读取文件
void RGop::read_video_by_index(short angle)
{
	uint32_t pos = start;//每次读1024字节
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


//接收客户端传过来的信息并且读取需要的文件块传输@!!!!!!测试模块
void RGop::receive_video_excute(string video_name)
{
	read_video_init(video_name);
	while(1)
	{
		//初始化socket
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
				memcpy(&angle, pic_buff, 2); //2字节的angle
											 //memcpy(&gop_num, pic_buff + 2, 4); //4字节的gop_num
											 //gop_num = ntohl(gop_num);
											 //angle = ntohs(angle);
				cout << "gop_num=" << gop_num << " angle=" << angle << endl;

				if (angle == -1 || gop_num == -1) {  //此时结束发送
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

	_LARGE_INTEGER time_start;    /*开始时间*/
	_LARGE_INTEGER time_over;        /*结束时间*/
	double dqFreq;                /*计时器频率*/
	LARGE_INTEGER f;            /*计时器频率*/
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
							cout << ((time_over.QuadPart - time_start.QuadPart) / dqFreq) << endl;//单位为秒，精度为1000 000/（cpu主频）微秒
							
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


