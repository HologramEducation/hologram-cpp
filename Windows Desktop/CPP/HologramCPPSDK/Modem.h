#pragma once
#include "Serial.h"

typedef struct _MODEM_INFO {
	std::wstring strManufacturer;
	std::wstring strModel;
	std::wstring strRevision;
	std::wstring strSVN;
	std::wstring strIMEI;

}MODEM_INFO;

//Modem class was adapted from the mobile broadband modem class found at
//https://github.com/northbright/WinUtil

class Modem : public Serial {
public:
	Modem();
	Modem(std::wstring port, DWORD baud);
	~Modem();

	//Serial Stuff
	virtual bool sendMessage(std::string message) = 0;
	virtual bool isRegistered() = 0;

	bool parseATCommandResult(std::string strATCommand, std::string & strOutput, std::vector<std::string >& resultArray);
	bool sendATCommand(std::string strATCommand, DWORD dwWaitTime = 500);
	bool sendATCommand(std::string strATCommand, std::string & strOutput, DWORD dwWaitTime = 500);
	bool sendAndParseATCommand(std::string strATCommand, std::vector<std::string >& resultArray, DWORD dwWaitTime = 500);

	bool isPDPContextActive();

	//RAS Stuff
	bool setupCellularDataConnection(LPWSTR modemName, LPWSTR connName);
	bool connect();

	RASCONNSTATUS getConnectionStatus() {
		rasConnStatus.dwSize = sizeof(RASCONNSTATUS);
		RasGetConnectStatus(hRasConn, &rasConnStatus);
		return rasConnStatus;
	}

	void disconnect() {
		if (hRasConn != NULL) {
			RasHangUp(hRasConn);
		}
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
	LPWSTR profileName;

	void updateConnectionStatus() {
		rasConnStatus.dwSize = sizeof(RASCONNSTATUS);
		RasGetConnectStatus(hRasConn, &rasConnStatus);
		connState = rasConnStatus.rasconnstate;
	}

	static void determineOFlags(DWORD flag);
	static void determineO2Flags(DWORD flag);
};

