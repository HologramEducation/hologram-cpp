#include "CustomCloud.h"

CustomCloud::CustomCloud(std::string sendHost, int sendPort, std::string recvHost, int recvPort) : Cloud(sendHost, sendPort, recvHost, recvPort)
{

}

CustomCloud::~CustomCloud()
{
}

std::wstring CustomCloud::sendMessage(std::wstring message, int timeout, bool closeSocket)
{
	if (!isReadyToSend()) {
		addPayloadToBuffer(message);
		return L"";
	}
	std::wstring result;
	openSendSocket(timeout);

	if (useATSocket) {
		result = L"AT Socket";
	}
	else {
		result = receiveSendSocket();
	}

	if (closeSocket) {
		closeSendSocket();
	}
	return result;
}
