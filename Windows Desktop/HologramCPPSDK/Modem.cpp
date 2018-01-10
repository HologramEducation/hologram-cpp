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

	ofStringReplace(strResult, "\r\nOK\r\n", "OK");
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
	TCHAR tchNewEntry[MAX_PATH + 1] = TEXT("\0");
	wsprintf(tchNewEntry, connName);
	profileName = connName;

	// Validate the entry name
	if (RasValidateEntryName(NULL, tchNewEntry) != ERROR_ALREADY_EXISTS) {

		// Set up the RASENTRY structure. Use the country/area codes
		RASENTRY rasEntry;
		DWORD dwResult = 0;
		memset(&rasEntry, 0, sizeof(RASENTRY));

		rasEntry.dwSize = sizeof(RASENTRY);
		wsprintf(rasEntry.szDeviceName, modemName);
		wsprintf(rasEntry.szDeviceType, RASDT_Modem);
		wsprintf(rasEntry.szLocalPhoneNumber, TEXT("*99***1#"));
		rasEntry.dwFramingProtocol = RASFP_Ppp;
		rasEntry.dwType = RASET_Phone;
		rasEntry.dwfNetProtocols = RASNP_Ip;
		rasEntry.dwEncryptionType = ET_Optional;
		rasEntry.dwfOptions = RASEO_IpHeaderCompression|RASEO_RemoteDefaultGateway|RASEO_SwCompression|RASEO_ModemLights;
		rasEntry.dwfOptions2 = RASEO2_SecureFileAndPrint|RASEO2_SecureClientForMSNet|RASEO2_DontUseRasCredentials|RASEO2_Internet|RASEO2_DisableNbtOverIP|RASEO2_IPv6RemoteDefaultGateway;

		// Create the entry
		dwResult = RasSetEntryProperties(NULL, tchNewEntry, &rasEntry,
			sizeof(RASENTRY), NULL, 0);

		// Check for any errors
		if (dwResult != 0) {
			TCHAR tchError[256] = TEXT("\0");

			// Print out the error
			wsprintf(tchError, TEXT("Could not create entry -- Error %ld"),
				dwResult);
			return false;
		}
	}
	else {
		//get the current settings and make sure they are right here
	}
	connState = RASCS_Disconnected;
	return true;
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
	wsprintf(rasDialParams.szEntryName, profileName);

	//auto retval = RasDial(NULL, NULL, &rasDialParams, 0L, (LPVOID)RasDialCallbackFunc, &hRasConn);
	auto retval = RasDial(NULL, NULL, &rasDialParams, 0L, NULL, &hRasConn);
	if (retval != SUCCESS) {
		wprintf(L"Encountered errer " + retval);
		return FALSE;
	}
	return true;
}

//static functions
std::vector<SERIAL_DEVICE_INFO> Modem::getConnectedSerialDevices() {
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