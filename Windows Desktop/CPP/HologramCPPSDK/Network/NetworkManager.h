#pragma once
#include "Base\Network.h"
#include "Modems\Base\Modem.h"
#include "Cellular.h"

#define DEFAULT_NETWORK_TIMEOUT  200

enum NetworkTypes {
	WIFI,
	CELLULAR,
	BLE,
	ETHERNET
};

class NetworkManager
{
public:
	NetworkManager(Network * network) {

	}
	~NetworkManager();

	void networkDisconnected() {
		active = false;
	}
	void networkConnected() {
		active = true;
	}

	void setNetworkType(NetworkTypes type, Modem * modem = NULL) {
		if (type == CELLULAR) {
			network = new Cellular();
			//set the network to cellular and detect or set modem
			if (modem != NULL) {
				((Cellular*)network)->modem = modem;
			}
			else {
				((Cellular*)network)->autoDectectModem();
			}
		}
		else {
			//create a new network of the type
		}
	}
	Network * getNetworkType() {
		return network;
	}

private:
	Network * network;
	bool active;
};

