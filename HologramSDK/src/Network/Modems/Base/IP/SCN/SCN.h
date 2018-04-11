#pragma once
#include "../Base/IP.h"
#include "../../Serial/Serial.h"
#ifdef TARGET_MACOS
#include <SystemConfiguration/SCNetwork.h>
#include <SystemConfiguration/SCNetworkConnection.h>
#include <SystemConfiguration/SCNetworkConfiguration.h>
#endif

class SCN : public IP
{
public:
	SCN(std::string name, Serial * serialport);
	~SCN();

    virtual bool connect();
	virtual void disconnect() {

	}
	virtual bool isConnected() {
		return false;
	}
private:
	Serial * serialport;
#ifdef TARGET_MACOS
    SCNetworkConnectionRef m_connection;
    CFStringRef serviceId;
#endif
};
