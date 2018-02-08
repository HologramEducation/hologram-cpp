#pragma once
#include "Base\Modem.h"
class Nova_R404 : public Modem
{
public:
	Nova_R404();
	~Nova_R404();

	virtual void setNetworkRegistrationStatus();
	virtual void initModemSerialMode();
	virtual bool isRegistered();
	virtual void handleURCSMS(std::string urcString);
	virtual void handleURCLocation(std::string urcString);
	virtual void handleURCListen(std::string urcString);
	void handleURC(std::string urcString);

	virtual void populateModemInformation();

	static SERIAL_DEVICE_INFO deviceInfo;

private:
	LOCATION location;
};

