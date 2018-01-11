// main.cpp : Defines the entry point for the console application.
//
#include "Modem.h"
#include "MobileBroadband.h"
#include <iostream>

int main(int argc, char **argv)
{
	wprintf(L"Mobile Broadband Information\n");
	MobileBroadbandManager mobileBroadbandManager;
	int nCount = mobileBroadbandManager.GetDeviceCount();
	if (!nCount)
		wprintf(L"No Mobile Broadband devices detected.\n");
	else
	{
		wprintf(L"%d Mobile Broadband devices detected.\n", nCount);
	}
	bool fRet = false;
	bool fOn = false;
	ULONG ulStrength = 0;
	for (int i = 0; i < nCount; i++)
	{
		fRet = mobileBroadbandManager.GetPowerState(i, fOn);
		fRet = mobileBroadbandManager.SetPowerState(i, !fOn);
		fRet = mobileBroadbandManager.GetPowerState(i, fOn);
		wprintf(L"id: %d, radioOn: %d, ", i, fOn);
		if (fOn)
		{
			fRet = mobileBroadbandManager.GetSignalStrength(i, ulStrength);
			if (fRet)
				wprintf(L"strength: %d\n", ulStrength);
			else
				wprintf(L"GetSignalStrength() failed\n");
		}
		else {
			wprintf(L"\n");
		}
	}

	wprintf(L"Checking for Connection Profiles\n");
	Modem::getConnectionProfiles();
	Modem nova;
	wprintf(L"Checking for Connected Modems\n");
	auto modems = Modem::getConnectedModems();
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
	wprintf(L"Setting up Dummy Connection Profile\n");
	nova.setupCellularConnection(L"u-blox modem", TEXT("Hologram Testing"));
	wprintf(L"\nTesting Serial Comms\n");
	std::vector<std::string> resultArray;
	wprintf(L"  Sending AT\n");
	nova.sendATCommand("AT", resultArray);
	for (auto result : resultArray) {
		wprintf(L"    Result: %S\n", result.c_str());
	}
	wprintf(L"  Sending ATH\n");
	nova.sendATCommand("ATH", resultArray);
	for (auto result : resultArray) {
		wprintf(L"    Result: %S\n", result.c_str());
	}
	wprintf(L"  Sending ATZ\n");
	nova.sendATCommand("ATZ", resultArray);
	for (auto result : resultArray) {
		wprintf(L"    Result: %S\n", result.c_str());
	}
	wprintf(L"  Sending ATQ0\n");
	nova.sendATCommand("ATQ0", resultArray);
	for (auto result : resultArray) {
		wprintf(L"    Result: %S\n", result.c_str());
	}
	wprintf(L"  Sending AT+CGDCONT=1,\"IP\",\"hologram\"\n");
	nova.sendATCommand("AT+CGDCONT=1,\"IP\",\"hologram\"", resultArray);
	for (auto result : resultArray) {
		wprintf(L"    Result: %S\n", result.c_str());
	}
	wprintf(L"  Sending ATDT*99***1#\n");
	nova.sendATCommand("ATDT*99***1#", resultArray);
	for (auto result : resultArray) {
		wprintf(L"    Result: %S\n", result.c_str());
	}
	Sleep(3000);
	wprintf(L"Hanging Up\n");
	nova.sendATCommand("ATH", resultArray);
	for (auto result : resultArray) {
		wprintf(L"    Result: %S\n", result.c_str());
	}
	wprintf(L"\nExiting\n\n");
	return 0;
}
