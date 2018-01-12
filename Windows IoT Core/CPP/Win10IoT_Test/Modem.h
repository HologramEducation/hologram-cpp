#pragma once
//#include "PPP.h"
#include "Utils.h"

typedef struct _MODEM_INFO {
	std::wstring strManufacturer;
	std::wstring strModel;
	std::wstring strRevision;
	std::wstring strSVN;
	std::wstring strIMEI;

}MODEM_INFO;

typedef struct _SERIAL_DEVICE_INFO {
	int vid, pid;
	std::wstring friendlyName;
	std::wstring portName;

}SERIAL_DEVICE_INFO;

class Modem {
public:
	Modem();
	~Modem();

	//Serial Stuff
	bool setupModemSerialPort(std::wstring lpszCom, DWORD dwBaudrate = 115200);
	bool setupSerialCommPort(std::wstring lpszCom, DWORD dwBaudrate = 115200);
	bool IsValid(HANDLE handle);

	// resultArray[0]: lpszATCommand
	// resultArray[1] - [n - 1]: output lines.
	bool parseATCommandResult(std::string strATCommand, std::string & strOutput, std::vector<std::string >& resultArray);
	bool sendATCommand(std::string strATCommand, std::string & strOutput, DWORD dwWaitTime = 250);
	bool sendATCommand(std::string strATCommand, std::vector<std::string >& resultArray, DWORD dwWaitTime = 250);

	bool getInfo(MODEM_INFO& modemInfo, DWORD dwWaitTime = 100);
	bool getIMSI(std::wstring& strIMSI, DWORD dwWaitTime = 100);

	static std::vector<SERIAL_DEVICE_INFO> getConnectedSerialDevices();

	bool connect();
	void disconnect();

private:
	HANDLE hModemCom;
	HANDLE hSerialCom;

	bool sendATCommand(HANDLE &handle, std::string strATCommand, std::string & strOutput, DWORD dwWaitTime = 100);
	bool sendATCommand(HANDLE &handle, std::string command, std::vector<std::string>& resultArray, DWORD dwWaitTime = 100);

	static bool parseVidPid(std::wstring deviceId, int* vid, int* pid)
	{
		const wchar_t* ptr = deviceId.c_str();
		const wchar_t* end = ptr + deviceId.size();
		// parse out the VID number
		const wchar_t* pos = wcsstr(ptr, L"VID_");
		if (pos == NULL) {
			wprintf(L"Failed to locate the VID");
			return false;
		}
		wchar_t* numberEnd = NULL;
		long c = wcstol(pos + 4, &numberEnd, 16);
		*vid = (int)c;

		// now the PID 
		pos = wcsstr(numberEnd, L"PID_");
		if (pos == NULL) {
			wprintf(L"Failed to locate the PID");
			return false;
		}

		numberEnd = NULL;
		c = wcstol(pos + 4, &numberEnd, 16);
		*pid = (int)c;

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
};

