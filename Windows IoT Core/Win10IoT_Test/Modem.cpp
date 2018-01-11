#include "Modem.h"

Modem::Modem()
{
	m_hCom = NULL;
	hRasConn = NULL;

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

bool Modem::setupCellularConnection(LPWSTR modemName, LPWSTR connName)
{
	wprintf(L"Attempting to create account %s with modem %s\n", connName, modemName);
	TCHAR tchNewEntry[MAX_PATH + 1] = TEXT("\0");
	StringCbPrintf(tchNewEntry, MAX_PATH + 1 * sizeof(TCHAR), TEXT("%s"), connName);
	profileName = connName;

	// Validate the entry name
	if (RasValidateEntryName(NULL, tchNewEntry) != ERROR_ALREADY_EXISTS) {
		wprintf(L"Did not find connection profile creating a new one\n");
		// Set up the RASENTRY structure. Use the country/area codes
		RASENTRY rasEntry;
		DWORD dwResult = 0;
		memset(&rasEntry, 0, sizeof(RASENTRY));

		rasEntry.dwSize = sizeof(RASENTRY);
		StringCbPrintf(rasEntry.szDeviceName, (MAX_PATH + 1) * sizeof(TCHAR), TEXT("%s"), modemName);
		StringCbPrintf(rasEntry.szDeviceType, 10 * sizeof(TCHAR), TEXT("%s"), RASDT_Modem);
		StringCbPrintf(rasEntry.szLocalPhoneNumber, 10 * sizeof(TCHAR), TEXT("%s"), TEXT("*99***1#"));
		rasEntry.dwFramingProtocol = RASFP_Ppp;
		rasEntry.dwType = RASET_Phone;
		rasEntry.dwfNetProtocols = RASNP_Ip;
		rasEntry.dwEncryptionType = ET_Optional;
		rasEntry.dwfOptions = RASEO_IpHeaderCompression | RASEO_RemoteDefaultGateway | RASEO_SwCompression | RASEO_ModemLights;
		rasEntry.dwfOptions2 = RASEO2_SecureFileAndPrint | RASEO2_SecureClientForMSNet | RASEO2_DontUseRasCredentials | RASEO2_Internet | RASEO2_DisableNbtOverIP | RASEO2_IPv6RemoteDefaultGateway;

		// Create the entry
		dwResult = RasSetEntryProperties(NULL, tchNewEntry, &rasEntry, sizeof(RASENTRY), NULL, 0);

		// Check for any errors
		if (dwResult != ERROR_SUCCESS) {
			switch (dwResult) {
			case ERROR_ACCESS_DENIED:
				wprintf(TEXT("The user does not have the correct privileges. Only an administrator can complete this task."));
				break;
			case ERROR_BUFFER_INVALID:
				wprintf(TEXT("The address or buffer specified by lpRasEntry is invalid."));
				break;
			case ERROR_CANNOT_OPEN_PHONEBOOK:
				wprintf(TEXT("The phone book is corrupted or missing components."));
				break;
			case ERROR_INVALID_PARAMETER:
				wprintf(TEXT("The RASENTRY structure pointed to by the lpRasEntry parameter does not contain adequate information, or the specified entry does not exist in the phone book."));
				break;
			case ERROR_FEATURE_DEPRECATED:
				wprintf(TEXT("A feature or setting you have tried to enable is no longer supported by the remote access service."));
				break;
			default:
				LPVOID lpMsgBuf;
				LPVOID lpDisplayBuf;

				FormatMessage(
					FORMAT_MESSAGE_ALLOCATE_BUFFER |
					FORMAT_MESSAGE_FROM_SYSTEM |
					FORMAT_MESSAGE_IGNORE_INSERTS,
					NULL,
					dwResult,
					MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
					(LPTSTR)&lpMsgBuf,
					0, NULL);

				// Display the error message and exit the process

				lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT,
					(lstrlen((LPCTSTR)lpMsgBuf) + lstrlen((LPCTSTR)L"RasSetEntryProperties") + 256) * sizeof(TCHAR));
				StringCchPrintf((LPTSTR)lpDisplayBuf,
					LocalSize(lpDisplayBuf) / sizeof(TCHAR),
					TEXT("%s failed with error %d: %s"),
					L"RasSetEntryProperties", dwResult, lpMsgBuf);
				wprintf(L"%s\n", lpDisplayBuf);
				LocalFree(lpMsgBuf);
				LocalFree(lpDisplayBuf);
				break;
			}
			return false;
		}
		wprintf(L"Successfully created account %s with modem %s", connName, modemName);
		connState = RASCS_Disconnected;
		return true;
	}
	else {
		//get the current settings and make sure they are right here
		wprintf(L"Profile already exists");
	}
	connState = RASCS_Disconnected;
	return false;
}

bool Modem::connect()
{
	// Dial a RAS entry in synchronous mode
	hRasConn = NULL;
	RASDIALPARAMS rasDialParams;

	// Setup the RASDIALPARAMS structure for the entry we want
	// to dial
	memset(&rasDialParams, 0, sizeof(RASDIALPARAMS));

	rasDialParams.dwSize = sizeof(RASDIALPARAMS);
	StringCbPrintf(rasDialParams.szEntryName, 150 * sizeof(TCHAR), TEXT("%s"), profileName);

	//auto retval = RasDial(NULL, NULL, &rasDialParams, 0L, (LPVOID)RasDialCallbackFunc, &hRasConn);
	auto retval = RasDial(NULL, NULL, &rasDialParams, 0L, NULL, &hRasConn);
	if (retval != SUCCESS) {
		wprintf(L"Encountered error %i\n", retval);
		return FALSE;
	}
	return true;
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

std::vector<LPRASDEVINFO> Modem::getConnectedModems() {

	DWORD dwCb = 0;
	DWORD dwRet = ERROR_SUCCESS;
	DWORD dwDevices = 0;
	LPRASDEVINFO lpRasDevInfo = NULL;
	std::vector<LPRASDEVINFO> devices;

	// Call RasEnumDevices with lpRasDevInfo = NULL. dwCb is returned with the required buffer size and 
	// a return code of ERROR_BUFFER_TOO_SMALL
	dwRet = RasEnumDevices(lpRasDevInfo, &dwCb, &dwDevices);

	if (dwRet == ERROR_BUFFER_TOO_SMALL) {
		// Allocate the memory needed for the array of RAS structure(s).
		lpRasDevInfo = (LPRASDEVINFO)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwCb);
		if (lpRasDevInfo == NULL) {
			wprintf(L"HeapAlloc failed!\n");
			return devices;
		}
		// The first RASDEVINFO structure in the array must contain the structure size
		lpRasDevInfo[0].dwSize = sizeof(RASDEVINFO);

		// Call RasEnumDevices to enumerate RAS devices
		dwRet = RasEnumDevices(lpRasDevInfo, &dwCb, &dwDevices);

		// If successful, print the names of the RAS devices
		if (ERROR_SUCCESS == dwRet) {
			wprintf(L"The following RAS Modems were found:\n");
			for (DWORD i = 0; i < dwDevices; i++) {
				wprintf(L"%s - %s\n", lpRasDevInfo[i].szDeviceName, lpRasDevInfo[i].szDeviceType);
				if (wcscmp(lpRasDevInfo[i].szDeviceType, RASDT_Modem) == 0) {
					wprintf(L"%s\n", lpRasDevInfo[i].szDeviceName);
					devices.push_back(&lpRasDevInfo[i]);
				}
			}
		}
		//Deallocate memory for the connection buffer
		HeapFree(GetProcessHeap(), 0, lpRasDevInfo);
		lpRasDevInfo = NULL;
		return devices;
	}

	// There was either a problem with RAS or there are no RAS devices to enumerate    
	if (dwDevices >= 1) {
		wprintf(L"The operation failed to acquire the buffer size.\n");
	}
	else {
		wprintf(L"There were no RAS devices found.\n");
	}

	return devices;
}

void Modem::getConnectionProfiles()
{
	DWORD dwErr = ERROR_SUCCESS;
	DWORD dwSize = 0, dwEntries;
	LPRASENTRYNAME pRasEntryName = NULL;
	LPRASENTRY pRasEntry = NULL;
	DWORD i;

	dwErr = RasEnumEntries(NULL, NULL, NULL, &dwSize, &dwEntries);

	if (dwErr == ERROR_BUFFER_TOO_SMALL)
	{
		pRasEntryName = (LPRASENTRYNAME)LocalAlloc(LPTR, dwSize);
		if (pRasEntryName == NULL)
		{
			printf("Alloc failed for EnumEntries %d", GetLastError());
			goto Done;
		}

		pRasEntryName->dwSize = sizeof(RASENTRYNAME);
		dwErr = RasEnumEntries(NULL, NULL, pRasEntryName, &dwSize, &dwEntries);
		if (dwErr != ERROR_SUCCESS)
		{
			printf("RasEnumEntries failed with error %d", dwErr);
			goto Done;
		}
		printf("Number of Entries %d\n", dwEntries);

		for (i = 0; i < dwEntries; i++)
		{
			dwSize = 0;
			dwErr = RasGetEntryProperties(pRasEntryName[i].szPhonebookPath,
				pRasEntryName[i].szEntryName,
				NULL,
				&dwSize,
				NULL, NULL);
			if (dwErr == ERROR_BUFFER_TOO_SMALL)
			{
				pRasEntry = (LPRASENTRY)LocalAlloc(LPTR, dwSize);
				if (pRasEntry == NULL)
				{
					printf("Alloc failed for RasEntry\n");
					goto Done;
				}

				pRasEntry->dwSize = sizeof(RASENTRY);
				dwErr = RasGetEntryProperties(pRasEntryName[i].szPhonebookPath,
					pRasEntryName[i].szEntryName,
					pRasEntry,
					&dwSize,
					NULL, 0);
				if (dwErr != ERROR_SUCCESS)
				{
					wprintf(L"RasGetEntryProperties failed with error %d for entry %s\n", dwErr, pRasEntryName[i].szEntryName);
					goto Done;
				}
			}
			else
			{
				wprintf(L"RasGetEntryProperties failed with error %d for entry %s\n", dwErr, pRasEntryName[i].szEntryName);
				goto Done;
			}
			determineOFlags(pRasEntry->dwfOptions);
			determineO2Flags(pRasEntry->dwfOptions2);
			LocalFree(pRasEntry);
			pRasEntry = NULL;
		}
	}
	else
	{
		goto Done;
	}

Done:
	if (pRasEntryName)
	{
		LocalFree(pRasEntryName);
	}
	if (pRasEntry)
	{
		LocalFree(pRasEntry);
	}
}

void Modem::determineOFlags(DWORD flag)
{
	if (flag & RASEO_UseCountryAndAreaCodes) {
		printf_s("RASEO_UseCountryAndAreaCodes\n");
	}
	if (flag & RASEO_SpecificIpAddr) {
		printf_s("RASEO_SpecificIpAddr\n");
	}
	if (flag & RASEO_SpecificNameServers) {
		printf_s("RASEO_SpecificNameServers\n");
	}
	if (flag & RASEO_IpHeaderCompression) {
		printf_s("RASEO_IpHeaderCompression\n");
	}
	if (flag & RASEO_RemoteDefaultGateway) {
		printf_s("RASEO_RemoteDefaultGateway\n");
	}
	if (flag & RASEO_DisableLcpExtensions) {
		printf_s("RASEO_DisableLcpExtensions\n");
	}
	if (flag & RASEO_TerminalBeforeDial) {
		printf_s("RASEO_TerminalBeforeDial\n");
	}
	if (flag & RASEO_TerminalAfterDial) {
		printf_s("RASEO_TerminalAfterDial\n");
	}
	if (flag & RASEO_ModemLights) {
		printf_s("RASEO_ModemLights\n");
	}
	if (flag & RASEO_SwCompression) {
		printf_s("RASEO_SwCompression\n");
	}
	if (flag & RASEO_RequireEncryptedPw) {
		printf_s("RASEO_RequireEncryptedPw\n");
	}
	if (flag & RASEO_RequireMsEncryptedPw) {
		printf_s("RASEO_RequireMsEncryptedPw\n");
	}
	if (flag & RASEO_RequireDataEncryption) {
		printf_s("RASEO_RequireDataEncryption\n");
	}
	if (flag & RASEO_NetworkLogon) {
		printf_s("RASEO_NetworkLogon\n");
	}
	if (flag & RASEO_UseLogonCredentials) {
		printf_s("RASEO_UseLogonCredentials\n");
	}
	if (flag & RASEO_PromoteAlternates) {
		printf_s("RASEO_PromoteAlternates\n");
	}
	if (flag & RASEO_SecureLocalFiles) {
		printf_s("RASEO_SecureLocalFiles\n");
	}
	if (flag & RASEO_RequireEAP) {
		printf_s("RASEO_RequireEAP\n");
	}
	if (flag & RASEO_RequirePAP) {
		printf_s("RASEO_RequirePAP\n");
	}
	if (flag & RASEO_RequireSPAP) {
		printf_s("RASEO_RequireSPAP\n");
	}
	if (flag & RASEO_Custom) {
		printf_s("RASEO_Custom\n");
	}
	if (flag & RASEO_PreviewPhoneNumber) {
		printf_s("RASEO_PreviewPhoneNumber\n");
	}
	if (flag & RASEO_SharedPhoneNumbers) {
		printf_s("RASEO_SharedPhoneNumbers\n");
	}
	if (flag & RASEO_PreviewUserPw) {
		printf_s("RASEO_PreviewUserPw\n");
	}
	if (flag & RASEO_PreviewDomain) {
		printf_s("RASEO_PreviewDomain\n");
	}
	if (flag & RASEO_ShowDialingProgress) {
		printf_s("RASEO_ShowDialingProgress\n");
	}
	if (flag & RASEO_RequireCHAP) {
		printf_s("RASEO_RequireCHAP\n");
	}
	if (flag & RASEO_RequireMsCHAP) {
		printf_s("RASEO_RequireMsCHAP\n");
	}
	if (flag & RASEO_RequireMsCHAP2) {
		printf_s("RASEO_RequireMsCHAP2\n");
	}
	if (flag & RASEO_RequireW95MSCHAP) {
		printf_s("RASEO_RequireW95MSCHAP\n");
	}
	if (flag & RASEO_CustomScript) {
		printf_s("RASEO_CustomScript\n");
	}
}

void Modem::determineO2Flags(DWORD flag)
{
	if (flag & RASEO2_SecureFileAndPrint) {
		printf_s("RASEO2_SecureFileAndPrint\n");
	}
	if (flag & RASEO2_SecureClientForMSNet) {
		printf_s("RASEO2_SecureClientForMSNet\n");
	}
	if (flag & RASEO2_DontNegotiateMultilink) {
		printf_s("RASEO2_DontNegotiateMultilink\n");
	}
	if (flag & RASEO2_DontUseRasCredentials) {
		printf_s("RASEO2_DontUseRasCredentials\n");
	}
	if (flag & RASEO2_UsePreSharedKey) {
		printf_s("RASEO2_UsePreSharedKey\n");
	}
	if (flag & RASEO2_Internet) {
		printf_s("RASEO2_Internet\n");
	}
	if (flag & RASEO2_DisableNbtOverIP) {
		printf_s("RASEO2_DisableNbtOverIP\n");
	}
	if (flag & RASEO2_UseGlobalDeviceSettings) {
		printf_s("RASEO2_UseGlobalDeviceSettings\n");
	}
	if (flag & RASEO2_ReconnectIfDropped) {
		printf_s("RASEO2_ReconnectIfDropped\n");
	}
	if (flag & RASEO2_SecureRoutingCompartment) {
		printf_s("RASEO2_SecureRoutingCompartment\n");
	}
	if (flag & RASEO2_UseTypicalSettings) {
		printf_s("RASEO2_UseTypicalSettings\n");
	}
	if (flag & RASEO2_IPv6SpecificNameServers) {
		printf_s("RASEO2_IPv6SpecificNameServers\n");
	}
	if (flag & RASEO2_IPv6RemoteDefaultGateway) {
		printf_s("RASEO2_IPv6RemoteDefaultGateway\n");
	}
	if (flag & RASEO2_RegisterIpWithDNS) {
		printf_s("RASEO2_RegisterIpWithDNS\n");
	}
	if (flag & RASEO2_UseDNSSuffixForRegistration) {
		printf_s("RASEO2_UseDNSSuffixForRegistration\n");
	}
	if (flag & RASEO2_IPv4ExplicitMetric) {
		printf_s("RASEO2_IPv4ExplicitMetric\n");
	}
	if (flag & RASEO2_IPv6ExplicitMetric) {
		printf_s("RASEO2_IPv6ExplicitMetric\n");
	}
	if (flag & RASEO2_DisableIKENameEkuCheck) {
		printf_s("RASEO2_DisableIKENameEkuCheck\n");
	}
	if (flag & RASEO2_DisableClassBasedStaticRoute) {
		printf_s("RASEO2_DisableClassBasedStaticRoute\n");
	}
	if (flag & RASEO2_SpecificIPv6Addr) {
		printf_s("RASEO2_SpecificIPv6Addr\n");
	}
	if (flag & RASEO2_DisableMobility) {
		printf_s("RASEO2_DisableMobility\n");
	}
	if (flag & RASEO2_RequireMachineCertificates) {
		printf_s("RASEO2_RequireMachineCertificates\n");
	}
	if (flag & RASEO2_UsePreSharedKeyForIkev2Initiator) {
		printf_s("RASEO2_UsePreSharedKeyForIkev2Initiator\n");
	}
	if (flag & RASEO2_UsePreSharedKeyForIkev2Responder) {
		printf_s("RASEO2_UsePreSharedKeyForIkev2Responder\n");
	}
	if (flag & RASEO2_CacheCredentials) {
		printf_s("RASEO2_CacheCredentials\n");
	}
	if (flag & RASEO2_AutoTriggerCapable) {
		printf_s("RASEO2_AutoTriggerCapable\n");
	}
	if (flag & RASEO2_IsThirdPartyProfile) {
		printf_s("RASEO2_IsThirdPartyProfile\n");
	}
	if (flag & RASEO2_AuthTypeIsOtp) {
		printf_s("RASEO2_AuthTypeIsOtp\n");
	}
	if (flag & RASEO2_IsAlwaysOn) {
		printf_s("RASEO2_IsAlwaysOn\n");
	}
	if (flag & RASEO2_IsPrivateNetwork) {
		printf_s("RASEO2_IsPrivateNetwork\n");
	}
}