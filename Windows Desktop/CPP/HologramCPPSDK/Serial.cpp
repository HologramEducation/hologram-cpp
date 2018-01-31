#include "Serial.h"

bool Serial::setupSerialPort(std::wstring port, DWORD baud)
{
	std::string strResult;
	DCB dcb = { 0 };

	if (m_hCom && m_hCom != INVALID_HANDLE_VALUE)
		CloseHandle(m_hCom);

	TCHAR pn[sizeof(port)];
	int num;
	if (swscanf_s(port.c_str(), L"COM%d", &num) == 1) {
		// Microsoft KB115831 a.k.a if COM > COM9 you have to use a different
		// syntax
		swprintf_s(pn, L"\\\\.\\COM%d", num);
	}
	else {
		wcsncpy_s(pn, (const TCHAR *)port.c_str(), sizeof(port) - 1);
	}

	m_hCom = CreateFile(pn, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
	if (m_hCom == INVALID_HANDLE_VALUE)
		return false;

	if (!GetCommState(m_hCom, &dcb))
		return false;

	dcb.BaudRate = baud;

	if (!SetCommState(m_hCom, &dcb))
		return false;

	return true;
}

bool Serial::write(std::string message)
{

	DWORD dwOut = 0;
	DWORD dwEvtMask = 0;

	if (!SetCommMask(m_hCom, EV_TXEMPTY)) {
		return false;
	}

	if (!WriteFile(m_hCom, message.data(), message.length(), &dwOut, NULL)) {
		return false;
	}

	WaitCommEvent(m_hCom, &dwEvtMask, NULL);  // Wait tx operation done.
}

bool Serial::read(std::string & buffer)
{
	DWORD dwOut = 0;
	char* pBuffer = NULL;
	DWORD dwErrors = 0;
	COMSTAT comStat = { 0 };

	if (!ClearCommError(m_hCom, &dwErrors, &comStat)) {
		return false;
	}

	if (!comStat.cbInQue) {
		return false;
	}

	pBuffer = new char[comStat.cbInQue + 1];
	if (!pBuffer) {
		return false;
	}

	memset(pBuffer, 0, comStat.cbInQue + 1);

	if (!ReadFile(m_hCom, pBuffer, comStat.cbInQue, &dwOut, NULL)) {
		if (pBuffer) {
			delete[] pBuffer;
			return false;
		}
	}
	
	buffer = pBuffer;

	return true;
}

bool Serial::IsInitialized()
{
	return (m_hCom && m_hCom != INVALID_HANDLE_VALUE) ? true : false;
}

//static functions
std::vector<SERIAL_DEVICE_INFO> Serial::getConnectedSerialDevices() {
	std::vector<SERIAL_DEVICE_INFO> devices;
	HDEVINFO hDevInfo = nullptr;
	SP_DEVINFO_DATA DeviceInterfaceData;
	DWORD dataType, actualSize = 0;

	// Search device set
	hDevInfo = SetupDiGetClassDevs((struct _GUID *)&GUID_SERENUM_BUS_ENUMERATOR, 0, 0, DIGCF_PRESENT);
	if (hDevInfo) {
		int i = 0;
		TCHAR dataBuf[MAX_PATH + 1];
		while (TRUE) {
			ZeroMemory(&DeviceInterfaceData, sizeof(DeviceInterfaceData));
			DeviceInterfaceData.cbSize = sizeof(DeviceInterfaceData);
			if (!SetupDiEnumDeviceInfo(hDevInfo, i, &DeviceInterfaceData)) {
				// SetupDiEnumDeviceInfo failed
				break;
			}

			if (SetupDiGetDeviceRegistryProperty(hDevInfo, &DeviceInterfaceData, SPDRP_FRIENDLYNAME,
				&dataType, (PBYTE)dataBuf, sizeof(dataBuf), &actualSize)) {

				TCHAR friendlyName[MAX_PATH + 1];
				TCHAR shortName[16];
				wsprintf(friendlyName, L"%s", dataBuf);

				// turn blahblahblah(COM4) into COM4

				TCHAR * begin = nullptr;
				TCHAR * end = nullptr;
				begin = wcsstr(dataBuf, L"COM");

				if (begin) {
					end = wcsstr(begin, L")");
					if (end) {
						*end = 0;   // get rid of the )...
						wcscpy_s(shortName, begin);
					}
				}
				SERIAL_DEVICE_INFO device;
				device.portName = shortName;
				device.friendlyName = friendlyName;
				devices.push_back(device);
			}
			i++;
		}
	}
	SetupDiDestroyDeviceInfoList(hDevInfo);
	return devices;
}