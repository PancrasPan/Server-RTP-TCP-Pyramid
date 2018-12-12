
#include <iostream>
#include "RtpSender.h"
using namespace std;
bool checkerror(int rtperr)
{
	if (rtperr < 0)
	{
		std::cout << "ERROR: " << RTPGetErrorString(rtperr) << std::endl;
		return false;
	}
	return true;
}


RtpSender::RtpSender(void)
{
	this->SetDefaultPayloadType(H264);//设置传输类型
	this->SetDefaultMark(true);		//设置位
	this->SetTimestampUnit(1.0 / 90000.0); //设置采样间隔
	this->SetDefaultTimestampIncrement(3000);//设置时间戳增加间隔
	dataMutex.Init();
	viewpointIndex = 0;
	connected = 0;
}


RtpSender::~RtpSender(void)
{
}

void RtpSender::OnAPPPacket(RTCPAPPPacket *apppacket, const RTPTime &receivetime, const RTPAddress *senderaddress)
{//收到RTCP APP数据
	std::cout << "Got RTCP packet from: " << senderaddress << std::endl;
	std::cout << "Got RTCP subtype: " << apppacket->GetSubType() << std::endl;
	std::cout << "Got RTCP data: " << (char *)apppacket->GetAPPData() << std::endl;
	return;
}

int RtpSender::SendPacket(const void* m_h264Buf, int buflen)
{
	const uint8_t *pSendbuf; //发送数据指针
	pSendbuf = (const uint8_t *)m_h264Buf;

	//去除前导码0x000001 或者0x00000001
	//if( 0x01 == m_h264Buf[2] )
	//{
	//	pSendbuf = &m_h264Buf[3];
	//	buflen -= 3;
	//}
	//else
	//{
	//	pSendbuf = &m_h264Buf[4];
	//	buflen -= 4;
	//}

	int status;
	printf("send packet length %d \n", buflen);
	//this->SetDefaultPayloadType()
	if (buflen <= MAX_RTP_PKT_LENGTH)
	{
		this->SetDefaultMark(true);
		status = RTPSession::SendPacket((void *)pSendbuf, buflen,H264,true,TIMESTAMP_INC);
		checkerror(status);


	}
	else if (buflen > MAX_RTP_PKT_LENGTH)
	{
		//设置标志位Mark为0
		this->SetDefaultMark(false);
		//得到该需要用多少长度为MAX_RTP_PKT_LENGTH字节的RTP包来发送
		int k = 0, l = 0;
		k = buflen / MAX_RTP_PKT_LENGTH;
		l = buflen % MAX_RTP_PKT_LENGTH;
		//int t = 0;//用指示当前发送的是第几个分片RTP包

		char nalHeader = pSendbuf[0]; // NALU 头
		do
		{
			if (k)//第一包到最后包的前一包
			{
				/*sendbuf[0] = (nalHeader & 0x60)|28;
				sendbuf[1] = (nalHeader & 0x1f);
				if ( 0 == t )
				{
				sendbuf[1] |= 0x80;
				}
				memcpy(sendbuf+2,&pSendbuf[t*MAX_RTP_PKT_LENGTH],MAX_RTP_PKT_LENGTH);
				status = this->SendPacket((void *)sendbuf,MAX_RTP_PKT_LENGTH+2);
				t++;
				*/
				status = RTPSession::SendPacket((void *)pSendbuf, MAX_RTP_PKT_LENGTH, H264, false, TIMESTAMP_INC);
				if (!checkerror(status))
					break;
				
				pSendbuf += MAX_RTP_PKT_LENGTH;
			}
			//最后一包
			else
			{
				//设置标志位Mark为1
				this->SetDefaultMark(true);
				int iSendLen = l?l: MAX_RTP_PKT_LENGTH;
				//sendbuf[0] = (nalHeader & 0x60)|28;  
				//sendbuf[1] = (nalHeader & 0x1f);
				//sendbuf[1] |= 0x40;
				//memcpy(sendbuf+2,&pSendbuf[t*MAX_RTP_PKT_LENGTH],iSendLen);
				//status = this->SendPacket((void *)sendbuf,iSendLen+2);
				status = RTPSession::SendPacket((void *)pSendbuf, iSendLen, H264, true, TIMESTAMP_INC);
				if (checkerror(status))
					break;
			}
			RTPTime::Wait(RTPTime(0, 5000));
		} while (k--);
	}
	return status;
}

int RtpSender::SendPacket(ifstream &f_video, int buflen)
{
	//const uint8_t *pSendbuf; //发送数据指针
	//pSendbuf = (const uint8_t *)m_h264Buf;
	char pSendbuf[MAX_RTP_PKT_LENGTH];
	int status;
	printf("send packet length %d \n", buflen);
	//this->SetDefaultPayloadType()
	if (buflen <= MAX_RTP_PKT_LENGTH)
	{
		this->SetDefaultMark(true);
		f_video.read(pSendbuf, buflen);
		status = RTPSession::SendPacket((void *)pSendbuf, buflen, H264, true, TIMESTAMP_INC);
		checkerror(status);
	}
	else if (buflen > MAX_RTP_PKT_LENGTH)
	{
		this->SetDefaultMark(false);
		int k = 0, l = 0;
		k = buflen / MAX_RTP_PKT_LENGTH;
		l = buflen % MAX_RTP_PKT_LENGTH;
		//int t = 0;//用指示当前发送的是第几个分片RTP包
		do
		{
			if (k)//第一包到最后包的前一包
			{
				f_video.read(pSendbuf, MAX_RTP_PKT_LENGTH);
				if (k == buflen / MAX_RTP_PKT_LENGTH)
				{
					for (int i = 0; i < 5; i++)
					{
						printf("%02x ", pSendbuf[i]);
					}
					printf("\n");
				}
				status = RTPSession::SendPacket((void *)pSendbuf, MAX_RTP_PKT_LENGTH, H264, false, TIMESTAMP_INC);
				if (!checkerror(status))
					break;

				//pSendbuf += MAX_RTP_PKT_LENGTH;
			}
			//最后一包
			else
			{
				//设置标志位Mark为1
				this->SetDefaultMark(true);
				int iSendLen = l ? l : MAX_RTP_PKT_LENGTH;
				f_video.read(pSendbuf, iSendLen);
				status = RTPSession::SendPacket((void *)pSendbuf, iSendLen, H264, true, TIMESTAMP_INC);
				if (!checkerror(status))
					break;
			}
			RTPTime::Wait(RTPTime(0, 3000));
		} while (k--);
	}
	return status;
}
int RtpSender::SendViewPoint(int viewpoint)
{
	int status = RTPSession::SendPacket((void *)&viewpoint, sizeof(int), VIEWPOINT, true, TIMESTAMP_INC);
	checkerror(status);
	return status;
}


void RtpSender::SetParams(uint16_t baseport)
{

	int status;
	//RTP+RTCP库初始化SOCKET环境
	RTPUDPv4TransmissionParams transparams;
	RTPSessionParams sessparams;

	sessparams.SetOwnTimestampUnit(1.0 / 9000.0); //时间戳单位
	sessparams.SetAcceptOwnPackets(true);	//接收自己发送的数据包
	sessparams.SetUsePredefinedSSRC(true);  //设置使用预先定义的SSRC
	sessparams.SetPredefinedSSRC(SSRC);     //定义SSRC
	transparams.SetPortbase(baseport);

	status = this->Create(sessparams, &transparams);
	checkerror(status);
}

void RtpSender::OnBYETimeout(RTPSourceData *srcdat)
{

}
void RtpSender::OnNewSource(RTPSourceData *dat)
{
	if (dat->IsOwnSSRC())
		return;

	uint32_t ip;
	uint16_t port;

	if (dat->GetRTPDataAddress() != 0)
	{
		const RTPIPv4Address *addr = (const RTPIPv4Address *)(dat->GetRTPDataAddress());
		ip = addr->GetIP();
		port = addr->GetPort();
		
	}
	else if (dat->GetRTCPDataAddress() != 0)
	{
		const RTPIPv4Address *addr = (const RTPIPv4Address *)(dat->GetRTCPDataAddress());
		ip = addr->GetIP();
		port = addr->GetPort() - 1;
	}
	else
		return;

	RTPIPv4Address dest(ip, port);
	AddDestination(dest);

	struct in_addr inaddr;
	inaddr.s_addr = htonl(ip);
	dataMutex.Lock();
	connected = true;
	dataMutex.Unlock();
	std::cout << "Adding destination " << std::string(inet_ntoa(inaddr)) << ":" << port << std::endl;
}

void RtpSender::OnBYEPacket(RTPSourceData *dat)
{
	if (dat->IsOwnSSRC())
		return;

	uint32_t ip;
	uint16_t port;

	if (dat->GetRTPDataAddress() != 0)
	{
		const RTPIPv4Address *addr = (const RTPIPv4Address *)(dat->GetRTPDataAddress());
		ip = addr->GetIP();
		port = addr->GetPort();
	}
	else if (dat->GetRTCPDataAddress() != 0)
	{
		const RTPIPv4Address *addr = (const RTPIPv4Address *)(dat->GetRTCPDataAddress());
		ip = addr->GetIP();
		port = addr->GetPort() - 1;
	}
	else
		return;

	RTPIPv4Address dest(ip, port);
	DeleteDestination(dest);

	struct in_addr inaddr;
	inaddr.s_addr = htonl(ip);
	dataMutex.Lock();
	connected = false;
	dataMutex.Unlock();
	std::cout << "Deleting destination " << std::string(inet_ntoa(inaddr)) << ":" << port << std::endl;
}

void RtpSender::OnRemoveSource(RTPSourceData *dat)
{
	if (dat->IsOwnSSRC())
		return;
	if (dat->ReceivedBYE())
		return;

	uint32_t ip;
	uint16_t port;

	if (dat->GetRTPDataAddress() != 0)
	{
		const RTPIPv4Address *addr = (const RTPIPv4Address *)(dat->GetRTPDataAddress());
		ip = addr->GetIP();
		port = addr->GetPort();
	}
	else if (dat->GetRTCPDataAddress() != 0)
	{
		const RTPIPv4Address *addr = (const RTPIPv4Address *)(dat->GetRTCPDataAddress());
		ip = addr->GetIP();
		port = addr->GetPort() - 1;
	}
	else
		return;

	RTPIPv4Address dest(ip, port);
	DeleteDestination(dest);

	struct in_addr inaddr;
	inaddr.s_addr = htonl(ip);
	std::cout << "Deleting destination " << std::string(inet_ntoa(inaddr)) << ":" << port << std::endl;
}

//void RtpSender::OnPollThreadStep()
//{
//	BeginDataAccess();
//	// check incoming packets
//	if (GotoFirstSourceWithData())
//	{
//		do
//		{
//			RTPPacket *pack;
//			RTPSourceData *srcdat;
//
//			srcdat = GetCurrentSourceInfo();
//
//			while ((pack = GetNextPacket()) != NULL)
//			{
//				ProcessRTPPacket(*srcdat, *pack);
//				DeletePacket(pack);
//			}
//		} while (GotoNextSourceWithData());
//	}
//
//	EndDataAccess();
//}
void RtpSender::ProcessRTPPacket(const RTPSourceData &srcdat, const RTPPacket &rtppack)
{
	// You can inspect the packet and the source's info here
	std::cout << "Got packet " << rtppack.GetExtendedSequenceNumber() << " from SSRC " << srcdat.GetSSRC() << std::endl;
	if (rtppack.GetPayloadType() == VIEWPOINT)
	{
		dataMutex.Lock();
		memcpy(&viewpointIndex, rtppack.GetPayloadData(), rtppack.GetPayloadLength());
		dataMutex.Unlock();
		printf("viewpoint index:%d\n", viewpointIndex);
	}

}