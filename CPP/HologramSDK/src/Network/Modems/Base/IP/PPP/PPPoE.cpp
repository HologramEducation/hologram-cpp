#include "PPPoE.h"

PPPoE::PPPoE(std::string name, std::string device)
{
	if (!socket_initialized_) {
		closed_ = true;
		socket_initialized_ = true;
#ifdef TARGET_WINDOWS
		WSADATA wsaData;
		// Initialize Winsock
		int rc = WSAStartup(MAKEWORD(2, 2), (LPWSADATA)&wsaData);
		if (rc != 0) {
			wprintf(L"WSAStartup failed with error : %d\n", rc);
		}
#endif
	}
}

PPPoE::~PPPoE()
{
	close();
}

void PPPoE::close()
{
	if (!closed_) {
		closed_ = true;

#ifdef TARGET_WINDOWS
		closesocket(sock);
#else
		int fd = static_cast<int>(sock);
		::close(fd);
#endif
	}
}

void PPPoE::connect(const std::string& localHost, int localPort, const std::string& remoteHost, int remotePort)
{
	sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	resolveAddress(localHost, localPort, localaddr);
	resolveAddress(remoteHost, remotePort, remoteaddr);

	// bind socket to local address.
	socklen_t addrlen = sizeof(sockaddr_in);
	int rc = bind(sock, reinterpret_cast<sockaddr*>(&localaddr), addrlen);
	if (rc < 0)
	{
#ifdef TARGET_WINDOWS
		int hr = WSAGetLastError();
		wprintf(L"PPPoE socket bind failed with error: %d\n", hr);
#endif
	}

	rc = ::connect(sock, reinterpret_cast<sockaddr*>(&remoteaddr), addrlen);
	if (rc != 0) {
#ifdef TARGET_WINDOWS
		int hr = WSAGetLastError();
		wprintf(L"PPPoE socket connect failed with error: %d\n", hr);
#endif
	}

	closed_ = false;
}

void PPPoE::accept(const std::string& localHost, int localPort)
{
#ifdef TARGET_WINDOWS
	SOCKET local = socket(AF_INET, SOCK_STREAM, 0);
#else
	int local = socket(AF_INET, SOCK_STREAM, 0);
#endif
	resolveAddress(localHost, localPort, localaddr);

	// bind socket to local address.
	socklen_t addrlen = sizeof(sockaddr_in);
	int rc = ::bind(local, reinterpret_cast<sockaddr*>(&localaddr), addrlen);
	if (rc < 0)
	{
#ifdef TARGET_WINDOWS
		int hr = WSAGetLastError();
		wprintf(L"PPPoE socket bind failed with error: %d\n", hr);
#endif
	}

	// start listening for incoming connection
	rc = ::listen(local, 1);
	if (rc < 0)
	{
#ifdef TARGET_WINDOWS

		int hr = WSAGetLastError();
		wprintf(L"PPPoE socket listen failed with error: %d\n", hr);
#endif
	}

	// accept 1
	sock = ::accept(local, reinterpret_cast<sockaddr*>(&remoteaddr), &addrlen);
	if (sock == INVALID_SOCKET) {
#ifdef TARGET_WINDOWS
		int hr = WSAGetLastError();
		wprintf(L"PPPoE accept failed with error: %d\n", hr);
#endif
	}

	closed_ = false;
}

int PPPoE::write(const uint8_t* ptr, int count)
{
	socklen_t addrlen = sizeof(sockaddr_in);
	int hr = send(sock, reinterpret_cast<const char*>(ptr), count, 0);
	if (hr == SOCKET_ERROR)
	{
		wprintf(L"PPPoE socket send failed with error: %d\n", hr);
	}

	return hr;
}

int
PPPoE::read(uint8_t* buffer, int bytesToRead)
{
	int bytesRead = 0;
	// try and receive something, up until port is closed anyway.

	while (!closed_)
	{
		socklen_t addrlen = sizeof(sockaddr_in);
		int rc = recv(sock, reinterpret_cast<char*>(buffer), bytesToRead, 0);
		if (rc < 0)
		{
#ifdef TARGET_WINDOWS
			int hr = WSAGetLastError();
			if (hr == WSAEMSGSIZE)
			{
				// message was too large for the buffer, no problem, return what we have.					
			}
			else if (hr == WSAECONNRESET || hr == ERROR_IO_PENDING)
			{
				// try again - this can happen if server recreates the socket on their side.
				continue;
			}
			else
#else
			int hr = errno;
			if (hr == EINTR)
			{
				// skip this, it is was interrupted.
				continue;
			}
			else
#endif
			{
				return -1;
			}
		}

		if (rc == 0)
		{
			//printf("Connection closed\n");
			return -1;
		}
		else
		{
			return rc;
		}
	}
	return -1;
}

bool PPPoE::isClosed()
{
	return closed_;
}

std::string PPPoE::remoteAddress()
{
	return inet_ntoa(remoteaddr.sin_addr);
}

int PPPoE::remotePort()
{
	return ntohs(remoteaddr.sin_port);
}

int PPPoE::getRssi(const char* ifaceName)
{
	return 0;
}

static void resolveAddress(const std::string& ipAddress, int port, sockaddr_in& addr)
{
#ifndef USERAS
	struct addrinfo hints;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);

	bool found = false;
	struct addrinfo *result = NULL;
	std::string serviceName = std::to_string(port);
	int rc = getaddrinfo(ipAddress.c_str(), serviceName.c_str(), &hints, &result);
	if (rc != 0) {
		wprintf(L"PPPoE getaddrinfo failed with error: %d\n", rc);
	}
	for (struct addrinfo *ptr = result; ptr != NULL; ptr = ptr->ai_next)
	{
		if (ptr->ai_family == AF_INET && ptr->ai_socktype == SOCK_STREAM && ptr->ai_protocol == IPPROTO_TCP)
		{
			// found it!
			sockaddr_in* sptr = reinterpret_cast<sockaddr_in*>(ptr->ai_addr);
			addr.sin_family = sptr->sin_family;
			addr.sin_addr.s_addr = sptr->sin_addr.s_addr;
			addr.sin_port = sptr->sin_port;
			found = true;
			break;
		}
	}

	freeaddrinfo(result);
	if (!found) {
		wprintf(L"PPPoE could not resolve ip address for '%s:%d'\n", ipAddress.c_str(), port);
	}
#endif
}