// main.cpp : Defines the entry point for the console application.
//
#include "Modem.h"

int main(int argc, char **argv)
{
	Modem nova;
	wprintf(L"Checking for Connected Serial Devices\n");
	for (auto device : Modem::getConnectedSerialDevices()) {
		wprintf(L"Found Serial Device: %s\n", device.friendlyName.c_str());
		if (wcsstr(device.portName.c_str(), L"0&0000") != nullptr) {
			if (nova.setupSerialPort(device.portName, 9600)) {
				wprintf(L"Connecting to Serial Device: %s on port %s\n", device.friendlyName.c_str(), device.portName.c_str());
				break;
			}
		}
	}
	if (nova.connect()) {
		
		nova.disconnect();
	}
	wprintf(L"\nExiting\n\n");
	return 0;
}
