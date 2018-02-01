#include "Modem.h"

Modem::Modem()
{
	m_hCom = NULL;
	hRasConn = NULL;

}

Modem::Modem(std::wstring port, DWORD baud) : Modem()
{
	setupSerialPort(port, baud);
}

Modem::~Modem()
{
	if (m_hCom && m_hCom != INVALID_HANDLE_VALUE) {
		CloseHandle(m_hCom);
	}
	disconnect();
}

bool Modem::parseATCommandResult(std::string strATCommand, std::string& strResult, std::vector<std::string>& resultArray)
{
	resultArray.clear();

	if (strResult.find("\r\nOK\r\n") == std::string::npos) {
		ofStringReplace(strResult, "\r\n", ";");
		ofStringReplace(strResult, "\r", "");
		resultArray = ofSplitString(strResult, ";");
		return false;
	}

	ofStringReplace(strResult, "\r\nOK\r\n", "OK");
	ofStringReplace(strResult, "\r\n", ";");
	ofStringReplace(strResult, "\r", "");

	resultArray = ofSplitString(strResult, ";");

	if (resultArray[0].find(strATCommand) == std::string::npos) {
		return false;
	}

	return true;
}

bool Modem::sendATCommand(std::string command, DWORD dwWaitTime)
{
	std::string strOutput;

	if (!sendATCommand(command, strOutput, dwWaitTime)) {
		return false;
	}
	std::vector<std::string> resultArray;
	return parseATCommandResult(command, strOutput, resultArray);
}

bool Modem::sendATCommand(std::string strATCommand, std::string& strOutput, DWORD dwWaitTime)
{
	if (!IsInitialized()) {
		return false;
	}

	if (!strATCommand.size()) {
		return false;
	}

	strATCommand = strATCommand +"\r\n";

	write(strATCommand);
	Sleep(dwWaitTime);  // Wait input buffer to be filled.
	read(strOutput);

	return true;
}

bool Modem::sendAndParseATCommand(std::string command, std::vector<std::string>& resultArray, DWORD dwWaitTime)
{
	std::string strOutput;

	if (!sendATCommand(command, strOutput, dwWaitTime)) {
		return false;
	}

	return parseATCommandResult(command, strOutput, resultArray);
}

bool Modem::isPDPContextActive()
{
	if (!isRegistered()) {
		return false;
	}

	std::vector<std::string > resultArray;
	if (sendAndParseATCommand("+UPSND 0,8", resultArray)) {

	}

	return false;
}

bool Modem::setupCellularDataConnection(LPWSTR modemName, LPWSTR connName)
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