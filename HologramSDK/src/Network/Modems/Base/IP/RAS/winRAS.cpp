#include "winRAS.h"

winRAS::winRAS(std::string name, std::string device)
{
#ifdef USERAS
	hRasConn = NULL;
	setupRASConnection(toWString(device), toWString(name));
#endif
}

winRAS::~winRAS()
{
#ifdef USERAS
	if (hRasConn != NULL) {
		RasHangUp(hRasConn);
	}
#endif
}


bool winRAS::connect()
{
#ifdef USERAS
	// Dial a RAS entry in synchronous mode
	hRasConn = NULL;
	RASDIALPARAMS rasDialParams;

	// Setup the RASDIALPARAMS structure for the entry we want
	// to dial
	memset(&rasDialParams, 0, sizeof(RASDIALPARAMS));

	rasDialParams.dwSize = sizeof(RASDIALPARAMS);
	wsprintf(rasDialParams.szEntryName, profileName.c_str());

	auto retval = RasDial(NULL, NULL, &rasDialParams, 0L, NULL, &hRasConn);
	if (retval != SUCCESS) {
		wprintf(L"Encountered error " + retval);
		return false;
	}
	return true;
#endif
    return false;
}

void winRAS::disconnect() {
#ifdef USERAS
	if (hRasConn != NULL) {
		RasHangUp(hRasConn);
	}
#endif
}

#ifdef USERAS
bool winRAS::setupRASConnection(std::wstring modemName, std::wstring connName)
{
	wchar_t tchNewEntry[MAX_PATH + 1] = TEXT("\0");
	wsprintf(tchNewEntry, connName.c_str());
	profileName = connName;

	// Validate the entry name
	if (RasValidateEntryName(NULL, tchNewEntry) != ERROR_ALREADY_EXISTS) {

		// Set up the RASENTRY structure
		RASENTRY rasEntry;
		DWORD dwResult = 0;
		memset(&rasEntry, 0, sizeof(RASENTRY));

		rasEntry.dwSize = sizeof(RASENTRY);
		wsprintf(rasEntry.szDeviceName, modemName.c_str());
		wsprintf(rasEntry.szDeviceType, RASDT_Modem);
		wsprintf(rasEntry.szLocalPhoneNumber, TEXT("*99***1#"));
		rasEntry.dwFramingProtocol = RASFP_Ppp;
		rasEntry.dwType = RASET_Phone;
		rasEntry.dwfNetProtocols = RASNP_Ip;
		rasEntry.dwEncryptionType = ET_Optional;
		rasEntry.dwfOptions = RASEO_IpHeaderCompression | RASEO_RemoteDefaultGateway | RASEO_SwCompression | RASEO_ModemLights;
		rasEntry.dwfOptions2 = RASEO2_SecureFileAndPrint | RASEO2_SecureClientForMSNet | RASEO2_DontUseRasCredentials | RASEO2_Internet | RASEO2_DisableNbtOverIP | RASEO2_IPv6RemoteDefaultGateway;

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
		// update the current settings and make sure they are right
		// Set up the RASENTRY structure
		RASENTRY rasEntry;
		DWORD dwResult = 0;
		memset(&rasEntry, 0, sizeof(RASENTRY));

		rasEntry.dwSize = sizeof(RASENTRY);
		wsprintf(rasEntry.szDeviceName, modemName.c_str());
		wsprintf(rasEntry.szDeviceType, RASDT_Modem);
		wsprintf(rasEntry.szLocalPhoneNumber, TEXT("*99***1#"));
		rasEntry.dwFramingProtocol = RASFP_Ppp;
		rasEntry.dwType = RASET_Phone;
		rasEntry.dwfNetProtocols = RASNP_Ip;
		rasEntry.dwEncryptionType = ET_Optional;
		rasEntry.dwfOptions = RASEO_IpHeaderCompression | RASEO_RemoteDefaultGateway | RASEO_SwCompression | RASEO_ModemLights;
		rasEntry.dwfOptions2 = RASEO2_SecureFileAndPrint | RASEO2_SecureClientForMSNet | RASEO2_DontUseRasCredentials | RASEO2_Internet | RASEO2_DisableNbtOverIP | RASEO2_IPv6RemoteDefaultGateway;

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
	connState = RASCS_Disconnected;
	return true;
}
#endif
