#include "Modem.h"

Modem::Modem()
{
	m_hCom = NULL;

}

Modem::~Modem()
{
	if (m_hCom && m_hCom != INVALID_HANDLE_VALUE)
		CloseHandle(m_hCom);
	disconnect();
}

bool Modem::IsValid()
{
	return (m_hCom && m_hCom != INVALID_HANDLE_VALUE) ? true : false;
}

bool Modem::setupSerialPort(std::wstring port, DWORD dwBaudrate)
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

	dcb.BaudRate = dwBaudrate;

	if (!SetCommState(m_hCom, &dcb))
		return false;

	return true;
}

bool Modem::parseATCommandResult(std::string strATCommand, std::string& strResult, std::vector<std::string>& resultArray)
{

	resultArray.clear();

	if (strResult.find("\r\nOK\r\n") == std::string::npos)
		return false;

	ofStringReplace(strResult, "\r\nOK\r\n", " OK");
	ofStringReplace(strResult, "\r\n", ";");
	ofStringReplace(strResult, "\r", "");

	resultArray = ofSplitString(strResult, ";");

	if (_stricmp(resultArray[0].c_str(), strATCommand.c_str()) != 0)
		return false;

	return true;
}

bool Modem::sendATCommand(std::string strATCommand, std::string& strOutput, DWORD dwWaitTime)
{
	DWORD dwOut = 0;
	char* pBuffer = NULL;
	OVERLAPPED ov = { 0 };
	DWORD dwErrors = 0;
	COMSTAT comStat = { 0 };
	DWORD dwEvtMask = 0;

	if (!IsValid()) {
		return false;
	}

	if (!strATCommand.size()) {
		return false;
	}

	strATCommand += "\r\n";

	if (!SetCommMask(m_hCom, EV_TXEMPTY)) {
		return false;
	}

	if (!WriteFile(m_hCom, strATCommand.data(), strATCommand.length(), &dwOut, NULL)) {
		return false;
	}

	WaitCommEvent(m_hCom, &dwEvtMask, NULL);  // Wait tx operation done.
	Sleep(dwWaitTime);  // Wait input buffer to be filled.

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

	strOutput = pBuffer;

	return true;
}

bool Modem::sendATCommand(std::string command, std::vector<std::string>& resultArray, DWORD dwWaitTime)
{
	std::string strOutput;

	if (!sendATCommand(command, strOutput, dwWaitTime)) {
		wprintf(L"Failed Sending AT Command: %S\n", command);
		return false;
	}

	return parseATCommandResult(command, strOutput, resultArray);
}

bool Modem::getInfo(MODEM_INFO& modemInfo, DWORD dwWaitTime)
{
	std::vector<std::string> lines;

	if (!IsValid())
		return false;

	if (!sendATCommand("ati", lines, dwWaitTime))
		return false;

	for (size_t i = 1; i < lines.size(); i++)
	{
		if (lines[i].find("Manufacturer: ") != std::string::npos)
			StringToWstring(CP_ACP, lines[i].substr(strlen("Manufacturer: ")), modemInfo.strManufacturer);
		else if (lines[i].find("Model: ") != std::string::npos)
			StringToWstring(CP_ACP, lines[i].substr(strlen("Model: ")), modemInfo.strModel);
		else if (lines[i].find("Revision: ") != std::string::npos)
			StringToWstring(CP_ACP, lines[i].substr(strlen("Revision: ")), modemInfo.strRevision);
		else if (lines[i].find("SVN: ") != std::string::npos)
			StringToWstring(CP_ACP, lines[i].substr(strlen("SVN: ")), modemInfo.strSVN);
		else if (lines[i].find("IMEI: ") != std::string::npos)
			StringToWstring(CP_ACP, lines[i].substr(strlen("IMEI: ")), modemInfo.strIMEI);
	}
	return true;
}

bool Modem::getIMSI(std::wstring& strIMSI, DWORD dwWaitTime)
{
	std::vector<std::string> lines;

	if (!IsValid())
		return false;

	if (!sendATCommand("at+cimi", lines, dwWaitTime))
		return false;

	return StringToWstring(CP_ACP, lines[1], strIMSI);
}

bool Modem::connect()
{
	if (!sendATCommand("AT", std::string())) {
		return false;
	}
	if (!sendATCommand("ATH", std::string())) {
		return false;
	}
	if (!sendATCommand("ATZ", std::string())) {
		return false;
	}
	if (!sendATCommand("ATQ0", std::string())) {
		return false;
	}
	if (!sendATCommand("AT+CGDCONT=1,\"IP\",\"hologram\"", std::string())) {
		return false;
	}
	if (!sendATCommand("ATDT*99***1#", std::string())) {
		return false;
	}
	PPP p;
	p.connect();
}

void Modem::disconnect()
{
	sendATCommand("ATH", std::string());
}

//static functions
std::vector<SERIAL_DEVICE_INFO> Modem::getConnectedSerialDevices() {
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
		info.pid = 0;
		info.vid = 0;
		info.portName = deviceInterface;
		int dvid = 0, dpid = 0;
		if (!parseVidPid(info.portName, &info.vid, &info.pid))
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