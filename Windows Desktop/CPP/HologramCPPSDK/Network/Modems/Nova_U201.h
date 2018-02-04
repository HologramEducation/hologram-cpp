#pragma once
#include "Base\Modem.h"
class Nova_U201 : public Modem
{
public:
	Nova_U201();
	~Nova_U201();

	bool createSocket();
	virtual void setNetworkRegistrationStatus();
	virtual void initModemSerialMode();
	virtual bool isRegistered();
	virtual void handleURCSMS(std::string urcString);
	virtual void handleURCLocation(std::string urcString);
	virtual void handleURCListen(std::string urcString);
	void handleURC(std::string urcString);

private:
	LOCATION location;
};

