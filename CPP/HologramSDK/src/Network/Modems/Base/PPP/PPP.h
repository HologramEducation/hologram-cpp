// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
#pragma once
#include "../../../../Utils/Utils.h"

#ifdef TARGET_WINDOWS
#pragma comment(lib, "Ws2_32.lib")
#include <winsock2.h>
#include <ws2tcpip.h>

#ifdef USERAS
#pragma comment(lib, "rasapi32.lib")
#include "ras.h"
#include "raserror.h"
#endif

typedef int socklen_t;

#else

#include <ctype.h>
#include <netdb.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/timeb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/ioctl.h>

#ifndef TARGET_ANDROID
#include <sys/signal.h>
#else
#include <signal.h>
#endif

//other types
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define FAR
#define SO_MAX_MSG_SIZE TCP_MAXSEG

#endif

class PPP 
{
public:
	PPP();
	~PPP();

	// Connect can set you up two different ways.  Pass 0 for local port to get any free local
	// port. localHost allows you to be specific about which local adapter to use in case you 
	// have multiple ethernet adapters. 
	void connect(const std::string& localHost, int localPort, const std::string& remoteHost, int remotePort);

	// start listening on the local adapter, and accept one connection request from a remote machine.
	void accept(const std::string& localHost, int localPort);

	// write the given bytes to the port, return number of bytes written or -1 if error.
	int write(const uint8_t* ptr, int count);

	// read some bytes from the port, return the number of bytes read or -1 if error.
	int read(uint8_t* buffer, int bytesToRead);

	// close the port.
	void close();

	bool isClosed();
    int getRssi(const char* ifaceName);
	std::string remoteAddress();
	int remotePort();
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
private:
	class PPPImpl;
	std::unique_ptr<PPPImpl> impl_;
	bool socket_initialized_;
};
