#pragma once
#include "Modem.h"

class Nova : public Modem {
public:
	Nova();
	~Nova();

	bool sendMessage(std::string message);

	bool isRegistered() {
		return true;
	}

	bool setNetworkRegistrationStatus() {
		return sendATCommand("+CEREG 2");
	}

private:
};