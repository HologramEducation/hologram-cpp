#pragma once
#include "Serial.h"

#ifdef TARGET_WIN32
#pragma comment(lib, "rasapi32.lib")
#include "ras.h"
#include "raserror.h"
#endif

#include "../../../Event/EventBus/EventBus.h"
#include "../../../Event/Events.h"

#include <deque>

#define RETRY_DELAY 50  // 50 millisecond delay to avoid spinning loops

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
	SOCKET_SEND_READ,
	SOCKET_CLOSED
};

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
	virtual std::string sendMessage(std::wstring message);

	//Cellular
	bool connect();
	void disconnect() {
#ifdef USERAS
		if (hRasConn != NULL) {
			RasHangUp(hRasConn);
		}
#endif
	}
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
	SMS popRecievedSMS();
	bool setSMSConfiguration();

	//MISC
	void setHexMode(bool state);
	std::string getName() {
		return name;
	}
	virtual void populateModemInformation() = 0;

	MODEM_INFO modemInfo;

#ifdef TARGET_WIN32

#ifdef USERAS
	//RAS Stuff
	bool setupRASConnection(std::wstring modemName, std::wstring connName);

	RASCONNSTATUS getConnectionStatus() {
		rasConnStatus.dwSize = sizeof(RASCONNSTATUS);
		RasGetConnectStatus(hRasConn, &rasConnStatus);
		return rasConnStatus;
	}

	RASCONNSTATE getConnectionState() {
		return connState;
	}

	static std::vector<LPRASDEVINFO> getConnectedModems();
	static void getConnectionProfiles();

private:
	HRASCONN hRasConn;
	RASCONNSTATE connState;
	RASCONNSTATUS rasConnStatus;
	std::wstring profileName;

	void updateConnectionStatus() {
		rasConnStatus.dwSize = sizeof(RASCONNSTATUS);
		RasGetConnectStatus(hRasConn, &rasConnStatus);
		connState = rasConnStatus.rasconnstate;
	}

	static void determineOFlags(DWORD flag);
	static void determineO2Flags(DWORD flag);
#endif

#endif
protected:
	bool checkRegistered(std::string atCommand);
	void parsePDU(std::string header, std::string pdu, SMS * sms, int & index);
	std::wstring parseSender(std::string pdu, int & offset);
	time_t parseTimestamp(std::string pdu, int & offset);
	std::wstring parseMessage(std::string pdu, int & offset);
	std::string name;
	URCState urcState;
	int last_read_payload_length;
	unsigned char socketId;
private:	
	std::deque<std::string> socketBuffer;

	ModemResult determineModemResult(std::string result);
};

