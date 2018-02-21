#pragma once
#include "Base\Cloud.h"
#include "../Event/EventBus/EventBus.h"
#include "../Event/Events.h"

#define MAX_RECEIVE_BYTES 1024
#define MAX_QUEUED_CONNECTIONS 5
#define RECEIVE_TIMEOUT 5
#define SEND_TIMEOUT 5000
#define MIN_PERIODIC_INTERVAL 1

class CustomCloud : public Cloud
{
public:
	CustomCloud(std::string sendHost, int sendPort, std::string recvHost, int recvPort);
	~CustomCloud();

	bool isReadyToSend();
	std::string sendMessage(std::wstring message, int timeout = SEND_TIMEOUT, bool closeSocket = true);
	void openSendSocket(int timeout = SEND_TIMEOUT);
	void closeSendSocket();
	std::string receiveSendSocket(unsigned int maxBytesRead = MAX_RECEIVE_BYTES);
	void openReceiveSocket();
	void closeReceiveSocket();

protected:
	bool useATSocket, sendSocketOpen;
};

