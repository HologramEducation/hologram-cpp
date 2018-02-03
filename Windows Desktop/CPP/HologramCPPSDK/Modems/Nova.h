#pragma once
#include "Base\Modem.h"

class Nova : public Modem {
public:
	Nova();
	~Nova();

	virtual bool sendMessage(std::wstring message);

	virtual bool isRegistered() {
		return true;
	}

	bool setNetworkRegistrationStatus() {
		return sendATCommand("AT+CEREG 2");
	}

private:
};