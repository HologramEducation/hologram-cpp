#pragma once
#include "../../Authentication/Base/Authentication.h"

#define VERSION "0.7.4"

class Cloud {
protected:
	Cloud(std::string sendHost, int sendPort, std::string recvHost, int recvPort) {
		this->sendHost = sendHost;
		this->sendPort = sendPort;
		this->recvHost = recvHost;
		this->recvPort = recvPort;
	}

	void initializeNetwork();
	void addPayloadToBuffer(std::wstring payload) {
		messageBuffer.push_back(payload);
	}
	virtual void sendMessage(std::wstring message, std::vector<std::wstring> topics) = 0;
	virtual void sendSMS(std::wstring message, std::vector<std::wstring> topics) = 0;


	Authentication * authenticator;
	int sendPort, recvPort;
	std::string sendHost, recvHost;
	std::vector<std::wstring> messageBuffer;
};