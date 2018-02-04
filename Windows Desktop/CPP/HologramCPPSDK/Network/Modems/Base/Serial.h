#pragma once
#include "../../Utils/Utils.h"
#include <setupapi.h>

typedef struct _SERIAL_DEVICE_INFO {
	std::wstring friendlyName;
	std::wstring portName;

}SERIAL_DEVICE_INFO;

class Serial {
public:
	bool setupSerialPort(std::wstring port, DWORD baud = 115200);
	bool write(std::string message);
	bool read(std::string &buffer);

	static std::vector<SERIAL_DEVICE_INFO> getConnectedSerialDevices();

protected:
	HANDLE m_hCom;

	bool IsInitialized();
};