#include "CustomCloud.h"

CustomCloud::CustomCloud(std::string sendHost, int sendPort, std::string recvHost, int recvPort) : Cloud(sendHost, sendPort, recvHost, recvPort)
{

}

CustomCloud::~CustomCloud()
{
}

std::string CustomCloud::sendMessage(std::wstring message, int timeout, bool closeSocket)
{
	if (!isReadyToSend()) {
		addPayloadToBuffer(message);
		return "";
	}
	std::string result;
	openSendSocket(timeout);

	if (useATSocket) {
		result = networkManager.getNetwork()->sendMessage(message);
	}
	else {
		result = receiveSendSocket();
	}

	if (closeSocket) {
		closeSendSocket();
	}
	return result;
}
