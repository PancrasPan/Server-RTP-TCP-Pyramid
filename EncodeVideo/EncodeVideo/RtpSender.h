#pragma once
#ifndef  RTP_SENDER
#define RTP_SENDER
//#pragma comment(lib, "ws2_32.lib")
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <jrtplib3/rtpsession.h>
#include <jrtplib3/rtpsessionparams.h>
#include <jrtplib3/rtpudpv4transmitter.h>
#include <jrtplib3/rtpipv4address.h>
#include <jrtplib3/rtptimeutilities.h>
#include <jrtplib3/rtppacket.h>
#include <jrtplib3/rtcpapppacket.h>
#include <jrtplib3/rtpsourcedata.h>
#include <stdlib.h>
#include <fstream>

using namespace std;
using namespace jrtplib;
#define MAX_RTP_PKT_LENGTH 1360
#define RX_BUF_SIZE 1356*1356

#define TIMESTAMP_UNIT 90000
#define FPS 30
#define TIMESTAMP_INC TIMESTAMP_UNIT/FPS

#define H264 96
#define VIEWPOINT 66
#define SSRC 100

class RtpSender :public RTPSession
{
public:
	RtpSender(void);
	~RtpSender(void);
	int SendPacket(const void * m_h264Buf, int buflen);
	int SendPacket(ifstream &f_videofp, int buflen);
	int SendViewPoint(int viewpoint);
	void SetParams(uint16_t baseport);
protected:
	void OnAPPPacket(RTCPAPPPacket *apppacket, const RTPTime &receivetime, const RTPAddress *senderaddress);
	void OnBYETimeout(RTPSourceData *srcdat);
	void OnNewSource(RTPSourceData *dat);
	void OnBYEPacket(RTPSourceData *dat);
	void OnRemoveSource(RTPSourceData *dat);
	//void OnPollThreadStep();
	void ProcessRTPPacket(const RTPSourceData &srcdat, const RTPPacket &rtppack);
public:
	int viewpointIndex;
	bool  connected;
	jthread::JMutex dataMutex;
};

bool checkerror(int rtperr);
#endif //  RTP_SENDER

