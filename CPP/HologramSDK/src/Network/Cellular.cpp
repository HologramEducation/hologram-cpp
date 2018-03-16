#include "Cellular.h"

Cellular::Cellular()
{
	supportedModems.push_back("NOVA201");
	supportedModems.push_back("NOVA404");
	supportedModems.push_back("E303");
	supportedModems.push_back("MS2131");
	
	

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
		EventBus::FireEvent(ConnectionEvent(NetworkType::CELLULAR));
		return true;
	}
	return false;
}

bool Cellular::disconnect()
{
	modem->disconnect();
	connectionState = CLOUD_DISCONNECTED;
	EventBus::FireEvent(DisconnectionEvent(NetworkType::CELLULAR));
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
#ifdef TARGET_WINDOWS
			if (Serial::isDeviceConnected(Nova_U201::deviceInfo, L"MI_02")) {
#else
			if (Serial::isDeviceConnected(Nova_R404::deviceInfo, L"2")) {
#endif
				modem = new Nova_U201();
				modem->setupSerialPort(Nova_U201::deviceInfo.portName);				
#ifdef USERAS
				for (auto device : Modem::getConnectedModems()) {
					if (wcsstr(device->szDeviceName, L"AT DATA")) {
						modem->setupRASConnection(device->szDeviceName, TEXT("Hologram Cellular Connection"));
						break;
					}
				}
#elif defined(IOTCORE)
				//IoT Core PPP code
#else
				//linux ppp code
#endif
				modem->initModemSerialMode();
				modem->populateModemInformation();
				break;
			}
		}
		if (modemName == "NOVA404") {
#ifdef TARGET_WINDOWS
			if (Serial::isDeviceConnected(Nova_R404::deviceInfo, L"MI_02")) {
#else
			if (Serial::isDeviceConnected(Nova_R404::deviceInfo, L"2")) {
#endif
				modem = new Nova_R404();
				modem->setupSerialPort(Nova_R404::deviceInfo.portName);
#ifdef USERAS
				for (auto device : Modem::getConnectedModems()) {
					if (wcsstr(device->szDeviceName, L"Qualcomm HS-USB Modem") != nullptr) {
						modem->setupRASConnection(device->szDeviceName, TEXT("Hologram Cellular Connection"));
						break;
					}
				}
#elif defined(IOTCORE)
				//IoT Core PPP code
#else
				//linux ppp code
#endif
				modem->initModemSerialMode();
				modem->populateModemInformation();
				break;
			}
		}
	}
}
