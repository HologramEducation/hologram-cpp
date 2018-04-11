#include "Modem.h"
#include <iostream>

Modem::Modem()
{
#ifdef TARGET_WINDOWS
	m_hCom = NULL;
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

std::string Modem::sendMessage(std::string message)
{
	urcState = SOCKET_INIT;

	writeSocket(message);

	while (urcState != SOCKET_SEND_READ && urcState != SOCKET_CLOSED) {
		checkURC();
		std::chrono::milliseconds timespan(RETRY_DELAY);
		std::this_thread::sleep_for(timespan);
	}
	if (urcState == SOCKET_SEND_READ) {
		EventBus::FireEvent(MessageReceivedEvent());
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

bool Modem::writeSocket(std::string data)
{
	setHexMode(true);
	std::vector<std::string> result;
	char buffer[4096];
	snprintf(buffer, 4096, "AT+USOWR=%d,%zd,\"%s\"", socketId, data.length(), toHex(data).c_str());
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
    } else if (urcString.find("+UUPSDD: ") != std::string::npos) {
        EventBus::FireEvent(DisconnectionEvent(CELLULAR));
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

			std::string sender = parseSender(pdu, offset);

			if (pdu.substr(offset, 4) == "0000") {
				offset += 4;

				time_t timestamp = parseTimestamp(pdu, offset);
				std::string message = parseMessage(pdu, offset);

				sms = new SMS(sender, timestamp, message);
			}
		}
	}
}

std::string Modem::parseSender(std::string pdu, int & offset)
{
	int sms_deliver = int(((unsigned char)fromHex(pdu.substr(offset, 1))[0]));
	std::string sender;
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
			sender = switchCharPairs(sender_raw);
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

std::string Modem::parseMessage(std::string pdu, int & offset)
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
#endif
