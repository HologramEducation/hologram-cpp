#pragma once
#include "../../Utils/Utils.h"

enum NetworkType {
	WIFI,
	CELLULAR,
	BLE,
	ETHERNET
};

enum ConnectionState {
	CLOUD_DISCONNECTED,
	CLOUD_CONNECTED,
	CLOUD_ERR_SIM,
	CLOUD_ERR_SIGNAL,
	CLOUD_ERR_CONNECT
};

class Network {
public:
	virtual bool connect(int timeout) = 0;
	virtual bool disconnect() = 0;
	virtual bool reconnect() = 0;
	virtual std::string sendMessage(std::string message) = 0;
	virtual bool openReceiveSocket(int recv_port) = 0;
	virtual bool createSocket() = 0;
	virtual bool connectSocket(std::string host, int port) = 0;
	virtual bool listenSocket(int port) = 0;
	virtual bool writeSocket(std::string data) = 0;
	virtual bool closeSocket() = 0;
	
	ConnectionState getConnectionStatus() {
		return connectionState;
	}
	virtual bool isConnected() = 0;
protected:
	ConnectionState connectionState;
};
