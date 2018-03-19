#include "Modem.h"

Modem::Modem()
{
#ifdef TARGET_WINDOWS
	m_hCom = NULL;
#ifdef USERAS
	hRasConn = NULL;
#endif
#else
	fd = -1;
#endif
	urcState = SOCKET_INIT;
}

Modem::~Modem()
{
#ifdef TARGET_WINDOWS
	if (m_hCom && m_hCom != INVALID_HANDLE_VALUE) {
		CloseHandle(m_hCom);
	}
#else
	tcsetattr(fd, TCSANOW, &oldoptions);
	::close(fd);
#endif
	disconnect();
}

ModemResult Modem::parseATCommandResult(std::string strATCommand, std::string& strResult, std::vector<std::string>& resultArray)
{
	resultArray.clear();

	ofStringReplace(strResult, "OK\r\n", "OK");
	ofStringReplace(strResult, "\r\n", ";");
	ofStringReplace(strResult, "\r", "");
	ofStringReplace(strResult, ";;", ";");

	while (strResult[0] == ';') {
		strResult.erase(0, 1);
	}

	resultArray = ofSplitString(strResult, ";");

	std::string result = resultArray.back();
	resultArray.pop_back();
	return determineModemResult(result);
}

bool Modem::sendATCommand(std::string command, unsigned int waitTIme)
{
	std::string strOutput;

	if (!sendATCommand(command, strOutput, waitTIme)) {
		return false;
	}
	std::vector<std::string> resultArray;
	return parseATCommandResult(command, strOutput, resultArray) == MODEM_OK;
}

bool Modem::sendATCommand(std::string strATCommand, std::string& strOutput, unsigned int waitTime)
{
	if (!IsInitialized()) {
		return false;
	}

	if (!strATCommand.size()) {
		return false;
	}

	checkURC();

	strATCommand = strATCommand + "\r\n";
	setTimeout(waitTime);
	if (write(strATCommand)) {
		read(strOutput, true);
	}
	else {
		return false;
	}
	return true;
}

ModemResult Modem::sendAndParseATCommand(std::string command, std::vector<std::string>& resultArray, unsigned int waitTIme)
{
	std::string strOutput;

	if (!sendATCommand(command, strOutput, waitTIme)) {
		return MODEM_ERROR;
	}

	return parseATCommandResult(command, strOutput, resultArray);
}

std::string Modem::sendMessage(std::wstring message)
{
	urcState = SOCKET_INIT;

	writeSocket(message);

	while (urcState != SOCKET_SEND_READ && urcState != SOCKET_CLOSED) {
		checkURC();
		std::chrono::milliseconds timespan(RETRY_DELAY);
		std::this_thread::sleep_for(timespan);
	}
	if (urcState == SOCKET_SEND_READ) {
		EventBus::FireEvent(MessageRecievedEvent());
		return readSocket(socketId, last_read_payload_length);
	}
	else {
		return "";
	}
}

std::string Modem::popRecievedMessage()
{
	checkURC();
	std::string data;
	if (socketBuffer.size() > 0) {
		data = socketBuffer.front();
		socketBuffer.pop_front();
	}
	return data;
}

bool Modem::isPDPContextActive()
{
	if (!isRegistered()) {
		return false;
	}

	std::vector<std::string> resultArray;
	if (sendAndParseATCommand("AT+UPSND=0,8", resultArray) == MODEM_OK) {
		return resultArray.back().back() == '1';
	}

	return false;
}

bool Modem::setupPDPContext()
{
	if (!isPDPContextActive()) {
		std::vector<std::string> resultArray;
		sendATCommand("AT+UPSD=0,1,\"hologram\"");
		sendATCommand("AT+UPSD=0,7,\"0.0.0.0\"");
		if (sendAndParseATCommand("AT+UPSDA=0,3", resultArray, 30000) == MODEM_OK) {
			return true;
		}
		return false;
	}
	return true;
}

void Modem::rebootModem()
{
	sendATCommand("AT+CFUN=16");
}

bool Modem::openReceiveSocket(int recv_port)
{
	if (createSocket()) {
		if (listenSocket(recv_port)) {
			return true;
		}
	}
	return false;
}

bool Modem::createSocket()
{
	std::vector<std::string> result;
	if (sendAndParseATCommand("AT+USOCR=6", result) == MODEM_OK) {
		socketId = std::stoi(ofSplitString(result[0], ":").back());
		return true;
	}
	return false;
}

bool Modem::connectSocket(std::string host, int port)
{
	std::vector<std::string> result;
	char buffer[1024];
	snprintf(buffer, 1024, "AT+USOCO=%d,\"%s\",%d", socketId, host.c_str(), port);
	return sendAndParseATCommand(buffer, result, 20000) == MODEM_OK;
}

bool Modem::listenSocket(int port)
{
	std::vector<std::string> result;
	char buffer[1024];
	snprintf(buffer, 1024, "AT+USOLI=%d,%d", socketId, port);
	return sendAndParseATCommand(buffer, result, 5000) == MODEM_OK;
}

bool Modem::writeSocket(std::wstring data)
{
	setHexMode(true);
	std::vector<std::string> result;
	char buffer[4096];
	snprintf(buffer, 4096, "AT+USOWR=%d,%zd,\"%s\"", socketId, data.length(), toHex(fromWString(data)).c_str());
	if (sendAndParseATCommand(buffer, result, 10000) != MODEM_OK) {
		//do something? notify or what
	}
	setHexMode(false);
	return true;
}

std::string Modem::readSocket(int socketID, int bufferLen)
{
	if (socketID < 0) {
		socketID = socketId;
	}

	if (bufferLen < 0) {
		bufferLen = last_read_payload_length;
	}

	setHexMode(true);
	std::vector<std::string> result;
	char buffer[4096];
	snprintf(buffer, 4096, "AT+USORD=%d,%d", socketID, bufferLen);
	sendAndParseATCommand(buffer, result);
	std::string response;
	if (result.size() > 0) {
		if (!result[0].empty()) {
			std::string atbuffer = buffer;
			response = result[0].substr(atbuffer.length() + 1, bufferLen * 2);
		}
	}

	response = fromHex(response);

	setHexMode(false);
	return response;
}

bool Modem::closeSocket(int socketID)
{
	if (socketID < 0) {
		socketID = socketId;
	}
	std::vector<std::string> result;
	return sendAndParseATCommand("AT+USOCL=" + std::to_string(socketID), result) == MODEM_OK;
}

void Modem::checkURC()
{
	while (true) {
		std::string buffer;
		read(buffer);
		if (buffer.find("\r\n+") != std::string::npos) {
			buffer = buffer.substr(2);
		}
		if (buffer.length() > 0 && buffer[0] == '+') {
			handleURC(buffer);
		}
		else {
			break;
		}
	}
}

void Modem::handleURC(std::string urcString)
{
	URCState nextState = urcState;
	if (urcString.find("+CMTI:") != std::string::npos) {
		handleURCSMS(urcString);
	}
	else if (urcString.find("+UULOC:") != std::string::npos) {
		handleURCLocation(urcString);
	}
	else if (urcString.find("+UUSORD:") != std::string::npos) {
		std::vector<std::string> response_list = ofSplitString(urcString.erase(0, 8), ",");
		int socket_identifier = std::stoi(response_list.front());
		int payload_length = std::stoi(response_list.back());

		if (urcState == SOCKET_RECEIVE_READ) {
			std::string message = readSocket(socket_identifier, payload_length);
			socketBuffer.push_back(message);
			closeSocket(socket_identifier);
		}
		else {
			socketId = socket_identifier;
			last_read_payload_length = payload_length;
			nextState = SOCKET_SEND_READ;
		}

	}
	else if (urcString.find("+UUSOLI: ") != std::string::npos) {
		handleURCListen(urcString);
		last_read_payload_length = 0;
		nextState = SOCKET_RECEIVE_READ;
	}
	else if (urcString.find("+UUSOCL: ") != std::string::npos) {
		socketId = urcString.back();
		last_read_payload_length = 0;
		nextState = SOCKET_CLOSED;
	}
	else {

	}
	urcState = nextState;
}

SMS Modem::popRecievedSMS()
{
	checkURC();
	std::vector<std::string> result;
	if (sendAndParseATCommand("AT+CMGL?", result, 20000) == MODEM_OK) {
		int oldestIndex = 0, currentIndex = 0;
		SMS * oldest = NULL, *current = NULL;
		for (unsigned int i = 0; i < result.size() - 1; i++) {
			parsePDU(result[i], result[i + 1], current, currentIndex);
			if (current != NULL) {
				if (oldest == NULL || current->timestamp < oldest->timestamp) {
					oldest = new SMS(*current);
					oldestIndex = currentIndex;
					if (oldestIndex > 0) {
						char buffer[32];
						snprintf(buffer, 32, "AT+CMGD=%d", oldestIndex);
						sendATCommand(buffer);
						return SMS(*oldest);
					}
				}
			}
		}
	}
	return SMS();
}

bool Modem::setSMSConfiguration()
{
	bool pduFormat = sendATCommand("AT+CMGF=0");
	bool newMessage = sendATCommand("AT+CNMI=2,1");
	return pduFormat & newMessage;
}

void Modem::setHexMode(bool state)
{
	std::string mode = (state ? "1" : "0");
	sendATCommand("AT+UDCONF=1," + mode);
}

bool Modem::connect()
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
		wprintf(L"Encountered errer " + retval);
		return FALSE;
	}
#else
	//we need to get the addresses and ports somehow
#endif
	return true;
}

bool Modem::setTimezoneConfiguration()
{
	bool zoneSync = sendATCommand("AT+CTZU=1");
	bool zoneURC = sendATCommand("AT+CTZR=1");
	return zoneSync & zoneURC;
}

bool Modem::checkRegistered(std::string atCommand)
{
	std::vector<std::string> result;
	if (sendAndParseATCommand(atCommand, result) == MODEM_OK) {
		std::vector<std::string> parts = ofSplitString(result.back().substr(result.back().find(":")), ",");
		if (parts.size() < 2) {
			return false;
		}
		return parts[1] == "5" || parts[1] == "1";
	}
	return false;
}

void Modem::parsePDU(std::string header, std::string pdu, SMS * sms, int & index)
{
	if (header.length() > 7 && header.substr(0, 7) == "+CMGL: ") {
		std::vector<std::string> headers = ofSplitString(header.substr(7), ",");
		if (headers.size() > 3) {
			std::string stat, alpha, length;
			index = stoi(headers.at(0));
			stat = headers.at(1);
			alpha = headers.at(2);
			length = headers.at(3);


			// parse PDU
			unsigned char hexVal = fromHex(pdu.substr(0, 2))[0];
			int smsc_len = int(hexVal); //convert from hex to int

			// smsc_number_type = int(pdu[2:4], 16)
			// if smsc_number_type != 0x81 and smsc_number_type != 0x91: return (-2, hex(smsc_number_type))
			int offset = smsc_len * 2 + 3;

			std::wstring sender = parseSender(pdu, offset);

			if (pdu.substr(offset, 4) == "0000") {
				offset += 4;

				time_t timestamp = parseTimestamp(pdu, offset);
				std::wstring message = parseMessage(pdu, offset);

				sms = new SMS(sender, timestamp, message);
			}
		}
	}
}

std::wstring Modem::parseSender(std::string pdu, int & offset)
{
	int sms_deliver = int(((unsigned char)fromHex(pdu.substr(offset, 1))[0]));
	std::wstring sender;
	if ((sms_deliver & 0x03) == 0) {
		offset += 1;
		int sender_len = int(((unsigned char)fromHex(pdu.substr(offset, 2))[0]));
		offset += 2;
		int sender_number_type = int(((unsigned char)fromHex(pdu.substr(offset, 2))[0]));
		offset += 2;
		int sender_read = sender_len;
		if ((sender_read & 1) != 0)
			sender_read += 1;
		std::string sender_raw = pdu.substr(offset, sender_read);
		if ((sender_number_type & 0x50) == 0x50) {
			//GSM - 7
			sender = convertGSM7to8bit(sender_raw, sender_len * 4 / 7);
		}
		else {
			//switch every pair of characters
			sender = toWString(switchCharPairs(sender_raw));
		}
		if ((sender_read & 1) != 0) {
			sender = sender.substr(0, sender.length() - 1);
		}
		offset += sender_read;
	}
	return sender;
}

time_t Modem::parseTimestamp(std::string pdu, int & offset)
{
	std::tm tm;
	std::string timestamp_raw = pdu.substr(offset, 14);
	//switch every pair of characters
	std::string timestamp = switchCharPairs(timestamp_raw);
	std::istringstream ss(timestamp);
	
	//datetime is given in this format '%y%m%d%H%M%S' 
	ss >> std::get_time(&tm, "%y%m%d%H%M%S");
	time_t unixTm = mktime(&tm);

	int tz_byte = int(((unsigned char)fromHex(pdu.substr(timestamp.length() - 2))[0]));
	int tz_bcd = ((tz_byte & 0x70) >> 4) * 10 + (tz_byte & 0x0F);

	int delta = 60 * 15 * tz_bcd; //15 minute intervals because there are weird timezones

	// adjust to UTC from Service Center timestamp
	if ((tz_byte & 0x80) == 0x80) {
		unixTm += delta;
	}
	else {
		unixTm -= delta;
	}
	return unixTm;
}

std::wstring Modem::parseMessage(std::string pdu, int & offset)
{
	offset += 14;
	int msg_len = int(((unsigned char)fromHex(pdu.substr(offset, 2))[0]));
	offset += 2;
	return convertGSM7to8bit(pdu.substr(offset), msg_len);
}

ModemResult Modem::determineModemResult(std::string result)
{
	if (result == "OK") {
		return MODEM_OK;
	}
	return MODEM_ERROR;
}

#ifdef USERAS
bool Modem::setupRASConnection(std::wstring modemName, std::wstring connName)
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
			for (unsigned int i = 0; i < dwDevices; i++) {
				if (wcscmp(lpRasDevInfo[i].szDeviceType, RASDT_Modem) == 0) {
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
	unsigned int i;

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
#endif
