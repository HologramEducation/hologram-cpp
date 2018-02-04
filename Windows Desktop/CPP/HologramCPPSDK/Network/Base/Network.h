#pragma once
#include "../../Utils/Utils.h"

enum ConnectionState {
	CLOUD_DISCONNECTED,
	CLOUD_CONNECTED,
	CLOUD_ERR_SIM,
	CLOUD_ERR_SIGNAL,
	CLOUD_ERR_CONNECT
};

class Network {
	virtual bool connect() = 0;
	virtual bool disconnect() = 0;
	virtual bool reconnect() = 0;
	virtual ConnectionState getConnectionStatus() {
		return connectionState;
	}
	virtual bool isConnected() = 0;
protected:
	std::string name;
	ConnectionState connectionState;
};