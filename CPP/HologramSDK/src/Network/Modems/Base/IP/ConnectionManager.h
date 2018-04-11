#pragma once
#include "Base/IP.h"
#include "PPP/PPP.h"
#include "RAS/winRAS.h"

enum ConnectionType {
	_PPP,
	_RAS
};

class ConnectionManager
{
public:
	ConnectionManager() {
		active = false;
	}

	ConnectionManager(IP * protocol) : ConnectionManager() {
		this->protocol = protocol;
	}
	~ConnectionManager() {
		delete protocol;
	}

	void setConnectionType(ConnectionType type, std::string name = "", std::string device = "", Serial * serialport = nullptr) {
		this->type = type;
		switch (type) {
		case _PPP:
			protocol = new PPP(serialport);
			break;
		case _RAS:
			protocol = new winRAS(name, device);
			break;
		default:
			break;
		}
	}
	IP * getConnection() {
		return protocol;
	}

	ConnectionType getConnectionType() {
		return type;
	}

private:
	IP * protocol;
	ConnectionType type;
	bool active;
};
