#pragma once
#include "../../Utils/Utils.h"
#include <setupapi.h>


typedef struct _SERIAL_DEVICE_INFO {
	_SERIAL_DEVICE_INFO() {}
	_SERIAL_DEVICE_INFO(std::wstring vid, std::wstring pid) {
		this->vid = vid;
		this->pid = pid;
	}
	std::wstring vid;
	std::wstring pid;
	std::wstring friendlyName;
	std::wstring portName;
}SERIAL_DEVICE_INFO;

class Serial {
public:
	bool setupSerialPort(std::wstring port, DWORD baud = 115200);
	bool write(std::string message);
	bool read(std::string &buffer);
	void setTimeout(int timeout);

	static bool isDeviceConnected(SERIAL_DEVICE_INFO & info, std::wstring name);
	static std::vector<SERIAL_DEVICE_INFO> getConnectedSerialDevices();

	static SERIAL_DEVICE_INFO deviceInfo;

protected:
	HANDLE m_hCom;
	bool IsInitialized();
};