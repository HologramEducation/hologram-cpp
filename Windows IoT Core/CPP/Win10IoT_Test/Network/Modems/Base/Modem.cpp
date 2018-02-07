#include "Modem.h"

Modem::Modem()
{
	m_hCom = NULL;
	urcState = SOCKET_INIT;
}

Modem::~Modem()
{
	if (m_hCom && m_hCom != INVALID_HANDLE_VALUE) {
		CloseHandle(m_hCom);
	}
	disconnect();
}

ModemResult Modem::parseATCommandResult(std::string strATCommand, std::string& strResult, std::vector<std::string>& resultArray)
{
	resultArray.clear();

	ofStringReplace(strResult, "\r\nOK\r\n", "OK");
	ofStringReplace(strResult, "\r\n", ";");
	ofStringReplace(strResult, "\r", "");

	if (strResult[0] == ';') {
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

	strATCommand = strATCommand + "\r\n";
	setTimeout(waitTime);
	write(strATCommand);
	read(strOutput, true);
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

	while (urcState != SOCKET_SEND_READ) {
		checkURC();
	}
	return readSocket(socketId, last_read_payload_length);
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
	sprintf_s(buffer, "AT+USOCO=%d,\"%s\",%d", socketId, host.c_str(), port);
	return sendAndParseATCommand(buffer, result, 5000) == MODEM_OK;
}

bool Modem::listenSocket(int port)
{
	std::vector<std::string> result;
	char buffer[1024];
	sprintf_s(buffer, "AT+USOLI=%d,%d", socketId, port);
	return sendAndParseATCommand(buffer, result, 5000) == MODEM_OK;
}

bool Modem::writeSocket(std::wstring data)
{
	setHexMode(true);
	std::vector<std::string> result;
	char buffer[4096];
	sprintf_s(buffer, "AT+USOWR=%d,%d,\"%s\"", socketId, data.length(), ToHex(wStringToString(data)).c_str());
	if (sendAndParseATCommand(buffer, result) != MODEM_OK) {
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
	sprintf_s(buffer, "AT+USORD=%d,%d", socketID, bufferLen);
	sendAndParseATCommand(buffer, result);
	std::string response;
	if (result.size() > 0) {
		response = result[0].substr(1, result[0].size() - 2);
	}

	response = hex2bin(response);

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
	else {

	}
	urcState = nextState;
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
	return false;
}

void Modem::disconnect()
{

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

ModemResult Modem::determineModemResult(std::string result)
{
	if (result == "OK") {
		return MODEM_OK;
	}
	return MODEM_ERROR;
}