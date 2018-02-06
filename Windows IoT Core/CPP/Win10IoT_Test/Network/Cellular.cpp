#include "Cellular.h"



Cellular::Cellular()
{
	supportedModems.push_back("E303");
	supportedModems.push_back("MS2131");
	supportedModems.push_back("NOVA201");
	supportedModems.push_back("NOVA404");

	connectionState = CLOUD_DISCONNECTED;
}


Cellular::~Cellular()
{
	delete modem;
}

bool Cellular::connect(int timeout)
{
	if (modem->connect()) {
		connectionState = CLOUD_CONNECTED;
		return true;
	}
	return false;
}

bool Cellular::disconnect()
{
	modem->disconnect();
	connectionState = CLOUD_DISCONNECTED;
	return true;
}

bool Cellular::reconnect()
{
	disconnect();
	return connect();
}

void Cellular::autoDectectModem()
{
	for (auto modemName : supportedModems) {
		if (modemName == "E303") {

		}
		if (modemName == "MS2131") {

		}
		if (modemName == "NOVA201") {
			if (Serial::isDeviceConnected(Nova_U201::deviceInfo, L"MI_02")) {
				modem = new Nova_U201();
				modem->setupSerialPort(Nova_U201::deviceInfo.portName);
				modem->initModemSerialMode(); 
				modem->populateModemInformation();
				break;
			}
		}
		if (modemName == "NOVA404") {

		}
	}
}
