#include "Serial.h"

bool Serial::setupSerialPort(std::wstring port, DWORD baud)
{
	std::string strResult;
	DCB dcb = { 0 };

	if (m_hCom && m_hCom != INVALID_HANDLE_VALUE)
		CloseHandle(m_hCom);

	m_hCom = CreateFile(port.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
	if (m_hCom == INVALID_HANDLE_VALUE)
		return false;

	if (!GetCommState(m_hCom, &dcb))
		return false;

	dcb.BaudRate = baud;

	if (!SetCommState(m_hCom, &dcb))
		return false;

	PurgeComm(m_hCom, PURGE_TXCLEAR | PURGE_TXABORT | PURGE_RXCLEAR | PURGE_RXABORT);

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
	return true;
}

bool Serial::read(std::string & buffer, bool waitForBuffer)
{
	DWORD dwOut = 0;
	DWORD dwEvtMask = 0;
	char* pBuffer = NULL;
	DWORD dwErrors = 0;
	COMSTAT comStat = { 0 };

	if (waitForBuffer) {
		if (!SetCommMask(m_hCom, EV_RXCHAR)) {
			return false;
		}

		WaitCommEvent(m_hCom, &dwEvtMask, NULL);  // Wait for the rx
	}

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

void Serial::setTimeout(int timeout)
{
	COMMTIMEOUTS timeouts;
	GetCommTimeouts(m_hCom, &timeouts);
	timeouts.ReadIntervalTimeout = 50;
	timeouts.ReadTotalTimeoutConstant = timeout;
	timeouts.ReadTotalTimeoutMultiplier = 50;
	SetCommTimeouts(m_hCom, &timeouts);
}

bool Serial::IsInitialized()
{
	return (m_hCom && m_hCom != INVALID_HANDLE_VALUE) ? true : false;
}

bool Serial::isDeviceConnected(SERIAL_DEVICE_INFO & info, std::wstring name)
{
	WCHAR CurrentDevice[MAX_DEVICE_ID_LEN];
	ULONG length;
	CONFIGRET cr = CM_Get_Device_Interface_List_Size(
		&length,
		const_cast<GUID*>(&GUID_DEVINTERFACE_COMPORT),
		nullptr,        // pDeviceID
		CM_GET_DEVICE_INTERFACE_LIST_PRESENT);

	if ((cr != CR_SUCCESS) || (length == 0)) {
		wprintf(L"Failed to get interface list size of COM ports\n");
		return false;
	}

	std::vector<WCHAR> buf(length);
	cr = CM_Get_Device_Interface_List(
		const_cast<GUID*>(&GUID_DEVINTERFACE_COMPORT),
		nullptr,        // pDeviceID
		buf.data(),
		static_cast<ULONG>(buf.size()),
		CM_GET_DEVICE_INTERFACE_LIST_PRESENT);

	if ((cr != CR_SUCCESS) || (length == 0) || !buf[0]) {
		wprintf(L"Failed to get interface list of COM ports\n");
		return false;
	}

	*buf.rbegin() = UNICODE_NULL;

	ULONG index = 0;
	for (PCWSTR deviceInterface = buf.data();
		*deviceInterface;
		deviceInterface += wcslen(deviceInterface) + 1) {

		const DEVPROPKEY propkey = {
			PKEY_DeviceInterface_Serial_PortName.fmtid,
			PKEY_DeviceInterface_Serial_PortName.pid
		};
		DEVPROPTYPE propertyType;
		WCHAR portName[512];
		ULONG propertyBufferSize = sizeof(portName);
		cr = CM_Get_Device_Interface_Property(
			deviceInterface,
			&propkey,
			&propertyType,
			reinterpret_cast<BYTE*>(&portName),
			&propertyBufferSize,
			0); // ulFlags

		if (isCorrectDevice(deviceInterface, info, name))
		{
			info.portName = deviceInterface;
			wprintf(L" for ");
			wprintf(deviceInterface);
			wprintf(L"\n");

			propertyBufferSize = sizeof(CurrentDevice);
			cr = CM_Get_Device_Interface_Property(deviceInterface,
				&DEVPKEY_Device_InstanceId,
				&propertyType,
				(PBYTE)CurrentDevice,
				&propertyBufferSize,
				0);

			if (cr != CR_SUCCESS)
			{
				wprintf(L"Failed getting the current device\n");
			}
			else if (propertyType != DEVPROP_TYPE_STRING)
			{
				wprintf(L"Property is not string\n");
			}
			else {
				info.friendlyName.assign(GetDeviceDescription(CurrentDevice));
				
			}
			return true;
		}
		++index;
	}
	return false;
}

//static functions
std::vector<SERIAL_DEVICE_INFO> Serial::getConnectedSerialDevices() {
	std::vector<SERIAL_DEVICE_INFO> devices;
	WCHAR CurrentDevice[MAX_DEVICE_ID_LEN];
	ULONG length;
	CONFIGRET cr = CM_Get_Device_Interface_List_Size(
		&length,
		const_cast<GUID*>(&GUID_DEVINTERFACE_COMPORT),
		nullptr,        // pDeviceID
		CM_GET_DEVICE_INTERFACE_LIST_PRESENT);

	if ((cr != CR_SUCCESS) || (length == 0)) {
		wprintf(L"Failed to get interface list size of COM ports\n");
		return devices;
	}

	std::vector<WCHAR> buf(length);
	cr = CM_Get_Device_Interface_List(
		const_cast<GUID*>(&GUID_DEVINTERFACE_COMPORT),
		nullptr,        // pDeviceID
		buf.data(),
		static_cast<ULONG>(buf.size()),
		CM_GET_DEVICE_INTERFACE_LIST_PRESENT);

	if ((cr != CR_SUCCESS) || (length == 0) || !buf[0]) {
		wprintf(L"Failed to get interface list of COM ports\n");
		return devices;
	}

	*buf.rbegin() = UNICODE_NULL;

	ULONG index = 0;
	for (PCWSTR deviceInterface = buf.data();
		*deviceInterface;
		deviceInterface += wcslen(deviceInterface) + 1) {

		const DEVPROPKEY propkey = {
			PKEY_DeviceInterface_Serial_PortName.fmtid,
			PKEY_DeviceInterface_Serial_PortName.pid
		};
		DEVPROPTYPE propertyType;
		WCHAR portName[512];
		ULONG propertyBufferSize = sizeof(portName);
		cr = CM_Get_Device_Interface_Property(
			deviceInterface,
			&propkey,
			&propertyType,
			reinterpret_cast<BYTE*>(&portName),
			&propertyBufferSize,
			0); // ulFlags


		SERIAL_DEVICE_INFO info;
		info.portName = deviceInterface;
		if (!parseVidPid(info.portName, info))
		{
			wprintf(L" for ");
			wprintf(deviceInterface);
			wprintf(L"\n");
		}
		else {
			wprintf(L"Found: ");
			wprintf(deviceInterface);
			wprintf(L"\n");
		}


		propertyBufferSize = sizeof(CurrentDevice);
		cr = CM_Get_Device_Interface_Property(deviceInterface,
			&DEVPKEY_Device_InstanceId,
			&propertyType,
			(PBYTE)CurrentDevice,
			&propertyBufferSize,
			0);

		if (cr != CR_SUCCESS)
		{
			wprintf(L"Failed getting the current device\n");
		}

		else if (propertyType != DEVPROP_TYPE_STRING)
		{
			wprintf(L"Property is not string\n");
		}
		else {
			info.friendlyName.assign(GetDeviceDescription(CurrentDevice));
		}

		devices.push_back(info);
		++index;
	}
	return devices;
}