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
	virtual bool isConnected() {
		return connectionState == CLOUD_CONNECTED;
	}

	void autoDectectModem();

	Modem * modem;
};

