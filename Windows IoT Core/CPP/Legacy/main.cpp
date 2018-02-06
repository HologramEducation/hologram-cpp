// main.cpp : Defines the entry point for the console application.
//
#include "Modem.h"
#include <iostream>

int main(int argc, char **argv)
{
	Modem nova;
	wprintf(L"Checking for Connected Serial Devices\n");
	for (auto device : Modem::getConnectedSerialDevices()) {
		wprintf(L"Found Serial Device: %s\n", device.friendlyName.c_str());
		if (wcsstr(device.portName.c_str(), L"0&0000") != nullptr) {
			if (nova.setupModemSerialPort(device.portName, 9600)) {
				wprintf(L"Connecting to Modem: %s on port %s\n", device.friendlyName.c_str(), device.portName.c_str());
			}
		}
		else if (wcsstr(device.portName.c_str(), L"0&0002") != nullptr) {
			if (nova.setupSerialCommPort(device.portName, 9600)) {
				wprintf(L"Connecting to Serial Device: %s on port %s\n", device.friendlyName.c_str(), device.portName.c_str());
			}
		}
	}
	if (nova.connect()) {
		wprintf(L"Connected\n");
		std::vector<std::string> resultArray;
		std::string input = "";
		while (input != "exit") {
			std::cout << "AT Command? ";
			std::getline(std::cin, input);
			if (input != "exit") {
				nova.sendATCommand(input, resultArray);
				for (auto result : resultArray) {
					std::cout << "Result: " << result << std::endl;
				}
			}
		}

	}
	nova.disconnect();
	wprintf(L"\nExiting\n\n");
	return 0;
}
