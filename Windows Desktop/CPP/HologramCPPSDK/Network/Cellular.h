#pragma once
#include "Base\Network.h"
#include "Modems\Base\Modem.h"
#include "Modems\Nova_U201.h"

#define DEFAULT_CELLULAR_TIMEOUT 200

std::string supportedModems[] = {
	"E303",
	"MS2131",
	"NOVA201",
	"NOVA404"
};

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
		modem->openReceiveSocket(recv_port);
	}
	virtual bool createSocket() {
		modem->createSocket();
	}
	virtual bool connectSocket(std::string host, int port) {
		modem->connectSocket(host, port);
		Sleep(2000);
	}
	virtual bool listenSocket(int port) {
		modem->listenSocket(port);
	}
	virtual bool writeSocket(std::wstring data) {
		modem->writeSocket(data);
	}
	virtual bool closeSocket() {
		modem->closeSocket(-1);
	}
	virtual bool isConnected() {
		return connectionState == CLOUD_CONNECTED;
	}

	void autoDectectModem();

	Modem * modem;
};

