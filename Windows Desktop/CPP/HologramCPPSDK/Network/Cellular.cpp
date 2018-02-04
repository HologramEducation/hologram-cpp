#include "Cellular.h"



Cellular::Cellular()
{
	Nova_U201::usbIds = USB_IDS(L"1546", L"1102");
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
			if (Serial::isDeviceConnected(Nova_U201::usbIds)) {
				modem = new Nova_U201();
				break;
			}
		}
		if (modemName == "NOVA404") {

		}
	}
}
