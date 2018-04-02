// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
#pragma once
#include "../Base/IP.h"

#ifdef TARGET_WINDOWS
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

class PPPoE
{
public:
	PPPoE(std::string name, std::string device);
	~PPPoE();

	// Connect can set you up two different ways.  Pass 0 for local port to get any free local
	// port. localHost allows you to be specific about which local adapter to use in case you 
	// have multiple ethernet adapters. 
	virtual void connect(const std::string& localHost, int localPort, const std::string& remoteHost, int remotePort);

	// start listening on the local adapter, and accept one connection request from a remote machine.
	virtual void accept(const std::string& localHost, int localPort);

	// write the given bytes to the port, return number of bytes written or -1 if error.
	virtual int write(const uint8_t* ptr, int count);

	// read some bytes from the port, return the number of bytes read or -1 if error.
	virtual int read(uint8_t* buffer, int bytesToRead);

	// close the port.
	virtual void close();

	virtual bool isClosed();
    virtual int getRssi(const char* ifaceName);
	virtual std::string remoteAddress();
	virtual int remotePort();
private:
#ifdef TARGET_WINDOWS
	SOCKET sock = INVALID_SOCKET;
#else
	int sock = 0;
#endif
	sockaddr_in localaddr;
	sockaddr_in remoteaddr;
	bool closed_;
	bool socket_initialized_;
};
