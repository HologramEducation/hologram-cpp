#pragma once
#include "../../../Utils/Utils.h"

#ifdef TARGET_WIN32
#pragma comment(lib, "Cfgmgr32.lib")
#include <cfgmgr32.h>
#include <initguid.h>
#include <propkey.h>
#include <devpkey.h>

//Modem GUID {2C7089AA-2E0E-11D1-B114-00C04FC2AAE4}
DEFINE_GUID(GUID_DEVINTERFACE_MODEM, 0x2C7089AA, 0x2E0E,
	0x11D1, 0xB1, 0x14, 0x00, 0xC0, 0x4F, 0xC2, 0xAA, 0xE4);
#else
#include <termios.h>
#include <sys/ioctl.h>
#include <getopt.h>
#include <dirent.h>
#include <errno.h>
#include <fstream>
#endif

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
	bool setupSerialPort(std::wstring port, unsigned int baud = 115200);
	bool write(std::string message);
	bool read(std::string &buffer, bool waitForBuffer = false);
	void setTimeout(int timeout);

	static bool isDeviceConnected(SERIAL_DEVICE_INFO & info, std::wstring name);
	static std::vector<SERIAL_DEVICE_INFO> getConnectedSerialDevices();
private:
	static bool parseVidPid(std::wstring device, SERIAL_DEVICE_INFO & deviceInfo)
	{
#ifdef TARGET_WIN32
		// parse out the VID number
		std::string::size_type vpos = device.find(L"VID_");
		std::string::size_type ppos = device.find(L"PID_");

		if (vpos == std::wstring::npos) {
			wprintf(L"Failed to locate the VID");
			return false;
		}

		// now the PID 
		if (ppos == std::wstring::npos) {
			wprintf(L"Failed to locate the PID");
			return false;
		}

		deviceInfo.vid = device.substr(vpos, 4);
		deviceInfo.pid = device.substr(ppos, 4);
#else
		std::string line;
		std::ifstream infile(device+L"idVendor");
		std::getline(infile, line);
		deviceInfo.vid = StringToWstring(line);
		
		infile = std::ifstream (device+L"idProduct");
		std::getline(infile, line);
		deviceInfo.pid = StringToWstring(line);
#endif
		return true;
	}
protected:
	bool IsInitialized();
#ifdef TARGET_WIN32
	HANDLE m_hCom;
private:
	static bool isCorrectDevice(std::wstring deviceId, SERIAL_DEVICE_INFO device, std::wstring name)
	{
		// parse out the VID number
		std::string::size_type pos = deviceId.find(L"VID_");
		if (pos == std::wstring::npos) {
			wprintf(L"Failed to locate the VID");
			return false;
		}
		if (device.vid != deviceId.substr(pos + 4, 4)) {
			//not the right device
			return false;
		}

		// now the PID 
		pos = deviceId.find(L"PID_");
		if (pos == std::wstring::npos) {
			wprintf(L"Failed to locate the PID");
			return false;
		}

		if (device.pid != deviceId.substr(pos + 4, 4)) {
			//not the right device
			return false;
		}

		if (deviceId.find(name) == std::wstring::npos) {
			return false;
		}

		return true;
	}

	static PWSTR GetDeviceDescription(PWSTR DEVICE_INSTID)
	{
		CONFIGRET cr = CR_SUCCESS;
		DEVINST Devinst;
		WCHAR DeviceDesc[2048];
		DEVPROPTYPE PropertyType;
		ULONG PropertySize;

		cr = CM_Locate_DevNode(&Devinst,
			DEVICE_INSTID,
			CM_LOCATE_DEVNODE_NORMAL);

		if (cr != CR_SUCCESS)
		{
			wprintf(L"Failed to locate the device with instance");
			wprintf(DEVICE_INSTID);
			wprintf(L"\n");
			return DeviceDesc;
		}

		// Query a property on the device.  For example, the device description.
		PropertySize = sizeof(DeviceDesc);
		cr = CM_Get_DevNode_Property(Devinst,
			&DEVPKEY_Device_DeviceDesc,
			&PropertyType,
			(PBYTE)DeviceDesc,
			&PropertySize,
			0);
		return DeviceDesc;
	}
#else
protected:
	int fd; 
	struct termios oldoptions;
#endif
};
