#pragma once
#include "../../Utils/Utils.h"
#include <setupapi.h>

typedef struct _USB_IDS {
	_USB_IDS(std::wstring vid, std::wstring pid) {
		this->vid = vid;
		this->pid = pid;
	}
	std::wstring vid;
	std::wstring pid;
}USB_IDS;

typedef struct _SERIAL_DEVICE_INFO {
	std::wstring friendlyName;
	std::wstring portName;
}SERIAL_DEVICE_INFO;

class Serial {
public:
	bool setupSerialPort(std::wstring port, DWORD baud = 115200);
	bool write(std::string message);
	bool read(std::string &buffer);

	static bool isDeviceConnected(USB_IDS ids);
	static std::vector<SERIAL_DEVICE_INFO> getConnectedSerialDevices();
	static USB_IDS usbIds;

protected:
	HANDLE m_hCom;
	bool IsInitialized();
};