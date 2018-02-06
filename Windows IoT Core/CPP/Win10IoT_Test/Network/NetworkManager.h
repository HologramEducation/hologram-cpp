#pragma once
#include "Base\Network.h"
#include "Modems\Base\Modem.h"
#include "Cellular.h"

#define DEFAULT_NETWORK_TIMEOUT  200

enum NetworkType {
	WIFI,
	CELLULAR,
	BLE,
	ETHERNET
};

class NetworkManager
{
public:
	NetworkManager() {
		active = false;
	}

	NetworkManager(Network * network) : NetworkManager() {
		this->network = network;
	}
	~NetworkManager() {
		delete network;
	}

	void networkDisconnected() {
		active = false;
	}
	void networkConnected() {
		active = true;
	}

	void setNetworkType(NetworkType type, Modem * modem = NULL) {
		this->type = type;
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
	Network * getNetwork() {
		return network;
	}

	NetworkType getNetworkType() {
		return type;
	}

private:
	Network * network;
	NetworkType type;
	bool active;
};

