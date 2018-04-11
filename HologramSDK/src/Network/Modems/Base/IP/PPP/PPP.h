#pragma once
#include "../Base/IP.h"
#include "../../Serial/Serial.h"

class PPP : public IP
{
public:
	PPP(Serial * serialport);
	~PPP();

	virtual bool connect() {
		return false;
	}
	virtual void disconnect() {

	}
	virtual bool isConnected() {
		return false;
	}
private:
	Serial * serialport;
};
