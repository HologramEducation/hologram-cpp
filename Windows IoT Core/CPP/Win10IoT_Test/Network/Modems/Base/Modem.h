#pragma once
#include "Serial.h"
#include <deque>

typedef struct _MODEM_INFO {
	std::wstring Manufacturer;
	std::wstring Model;
	std::wstring Revision;
	std::wstring SVN;
	std::wstring IMEI;
	std::wstring ICCID;
	std::wstring Mode;
}MODEM_INFO;

enum ModemResult {
	MODEM_OK,
	MODEM_TIMEOUT,
	MODEM_INVALID,
	MODEM_NO_MATCH,
	MODEM_ERROR
};

enum URCState {
	SOCKET_INIT,
	SOCKET_WRITE_STATE,
	SOCKET_RECEIVE_READ,
	SOCKET_SEND_READ
};

//Modem class was adapted from the mobile broadband modem class found at
//https://github.com/northbright/WinUtil

class Modem : public Serial {
public:
	Modem();
	~Modem();

	//Stuff subclasses have to implement
	virtual bool isRegistered() = 0;

	//Serial
	ModemResult parseATCommandResult(std::string strATCommand, std::string & strOutput, std::vector<std::string >& resultArray);
	bool sendATCommand(std::string strATCommand, unsigned int waitTIme = 250);
	bool sendATCommand(std::string strATCommand, std::string & strOutput, unsigned int waitTIme = 250);
	ModemResult sendAndParseATCommand(std::string strATCommand, std::vector<std::string >& resultArray, unsigned int waitTIme = 250);
	virtual void initModemSerialMode() = 0;

	//Hologram
	std::string sendMessage(std::wstring message);

	//Cellular
	bool connect();
	void disconnect();
	bool setTimezoneConfiguration();
	virtual void setNetworkRegistrationStatus() = 0;
	std::string popRecievedMessage();
	bool isPDPContextActive();
	bool setupPDPContext();
	void rebootModem();
	bool isConnected() {
		return isRegistered();
	}

	//Socket
	bool openReceiveSocket(int recv_port);
	virtual bool createSocket();
	bool connectSocket(std::string host, int port);
	bool listenSocket(int port);
	bool writeSocket(std::wstring data);
	std::string readSocket(int socketID, int bufferLen);
	bool closeSocket(int socketID);

	//URC
	void checkURC();
	void handleURC(std::string urcString);
	virtual void handleURCSMS(std::string urcString) = 0;
	virtual void handleURCLocation(std::string urcString) = 0;
	virtual void handleURCListen(std::string urcString) = 0;

	//SMS
	bool popRecievedSMS();
	bool setSMSConfiguration();
	bool enableSMS(bool state);

	//MISC
	void setHexMode(bool state);
	std::string getName() {
		return name;
	}
	virtual void populateModemInformation() = 0;

	MODEM_INFO modemInfo;
protected:
	bool checkRegistered(std::string atCommand);
	std::string name;
private:
	std::wstring profileName;

	unsigned char socketId;
	URCState urcState;
	int last_read_payload_length;
	std::deque<std::string> socketBuffer;

	ModemResult determineModemResult(std::string result);
};

