#pragma once
#include "Base\Network.h"
#include "Modems\Base\Modem.h"
#include "Modems\Nova_U201.h"
#include "Modems\Nova_R404.h"

#define DEFAULT_CELLULAR_TIMEOUT 200

class Cellular : public Network
{
public:
	Cellular();
	~Cellular();

	virtual bool connect(int timeout = DEFAULT_CELLULAR_TIMEOUT);
	virtual bool disconnect();
	virtual bool reconnect();
	virtual std::string sendMessage(std::wstring message) {
		return modem->sendMessage(message);
	}
	virtual bool openReceiveSocket(int recv_port) {
		return modem->openReceiveSocket(recv_port);
	}
	virtual bool createSocket() {
		return modem->createSocket();
	}
	virtual bool connectSocket(std::string host, int port) {
		bool status = modem->connectSocket(host, port);
		Sleep(2000);
		return status;
	}
	virtual bool listenSocket(int port) {
		return modem->listenSocket(port);
	}
	virtual bool writeSocket(std::wstring data) {
		return modem->writeSocket(data);
	}
	virtual bool closeSocket() {
		return modem->closeSocket(-1);
	}
	virtual bool isConnected() {
		return connectionState == CLOUD_CONNECTED || modem->isConnected();
	}

	void autoDectectModem();

	Modem * modem;

private:
	std::vector<std::string> supportedModems;
};

