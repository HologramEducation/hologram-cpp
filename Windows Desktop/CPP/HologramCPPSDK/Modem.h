#pragma once
#include "Utils.h"

typedef struct _MODEM_INFO {
	std::wstring strManufacturer;
	std::wstring strModel;
	std::wstring strRevision;
	std::wstring strSVN;
	std::wstring strIMEI;

}MODEM_INFO;

typedef struct _SERIAL_DEVICE_INFO {
	std::wstring friendlyName;
	std::wstring portName;

}SERIAL_DEVICE_INFO;

//Modem class was adapted from the mobile broadband modem class found at
//https://github.com/northbright/WinUtil

class Modem {
public:
	Modem();
	~Modem();

	//Serial Stuff
	bool setupSerialPort(std::wstring lpszCom, DWORD dwBaudrate = 115200);
	bool IsValid();

	// resultArray[0]: lpszATCommand
	// resultArray[1] - [n - 1]: output lines.
	bool parseATCommandResult(std::string strATCommand, std::string & strOutput, std::vector<std::string >& resultArray);
	bool sendATCommand(std::string strATCommand, std::string & strOutput, DWORD dwWaitTime = 100);
	bool sendATCommand(std::string strATCommand, std::vector<std::string >& resultArray, DWORD dwWaitTime = 100);

	bool getInfo(MODEM_INFO& modemInfo, DWORD dwWaitTime = 100);
	bool getIMSI(std::wstring& strIMSI, DWORD dwWaitTime = 100);

	static std::vector<SERIAL_DEVICE_INFO> getConnectedSerialDevices();

	//RSA Stuff
	bool setupCellularConnection(LPWSTR modemName, LPWSTR connName);
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
	void updateConnectionStatus() {
		rasConnStatus.dwSize = sizeof(RASCONNSTATUS);
		RasGetConnectStatus(hRasConn, &rasConnStatus);
		connState = rasConnStatus.rasconnstate;
	}
	RASCONNSTATE getConnectionState() {
		return connState;
	}

	static std::vector<LPRASDEVINFO> getConnectedModems();
	static void getConnectionProfiles();
	static void determineOFlags(DWORD flag);
	static void determineO2Flags(DWORD flag);
private:
	HANDLE m_hCom;
	HRASCONN hRasConn;
	RASCONNSTATE connState;
	RASCONNSTATUS rasConnStatus;
	LPWSTR profileName;
};

