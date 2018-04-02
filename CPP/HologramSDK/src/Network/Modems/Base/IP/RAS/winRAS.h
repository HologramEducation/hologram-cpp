#pragma once
#include "../Base/IP.h"

#pragma comment(lib, "rasapi32.lib")
#include "ras.h"
#include "raserror.h"

class winRAS : public IP
{
public:
	winRAS(std::string name, std::string device);
	~winRAS();

	virtual bool connect();
	virtual void disconnect();
	virtual bool isConnected() {
		updateConnectionState();
		return getConnectionState() == RASCS_Connected;
	}

	bool setupRASConnection(std::wstring modemName, std::wstring connName);



private:
	HRASCONN hRasConn;
	RASCONNSTATE connState;
	RASCONNSTATUS rasConnStatus;
	std::wstring profileName;

	void updateConnectionState() {
		rasConnStatus.dwSize = sizeof(RASCONNSTATUS);
		RasGetConnectStatus(hRasConn, &rasConnStatus);
		connState = rasConnStatus.rasconnstate;
	}	
	
	RASCONNSTATUS getConnectionStatus() {
		rasConnStatus.dwSize = sizeof(RASCONNSTATUS);
		RasGetConnectStatus(hRasConn, &rasConnStatus);
		return rasConnStatus;
	}

	RASCONNSTATE getConnectionState() {
		return connState;
	}
};

