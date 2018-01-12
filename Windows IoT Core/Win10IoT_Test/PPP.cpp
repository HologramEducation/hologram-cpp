// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "PPP.h"

typedef int socklen_t;
static bool socket_initialized_ = false;

class PPP::PPPImpl
{
	SOCKET sock = INVALID_SOCKET;
	sockaddr_in localaddr;
	sockaddr_in remoteaddr;
	bool closed_ = true;
public:

	bool isClosed() {
		return closed_;
	}

    int getRssi(const char* ifaceName)
    {
        return 0;
    }
	static void resolveAddress(const std::string& ipAddress, int port, sockaddr_in& addr)
	{
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
			wprintf(L"PPP getaddrinfo failed with error: %d\n", rc);
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
			wprintf(L"PPP could not resolve ip address for '%s:%d'\n", ipAddress.c_str(), port);
		}
	}

	int connect(const std::string& localHost, int localPort, const std::string& remoteHost, int remotePort)
	{
		sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

		resolveAddress(localHost, localPort, localaddr);
		resolveAddress(remoteHost, remotePort, remoteaddr);

		// bind socket to local address.
		socklen_t addrlen = sizeof(sockaddr_in);
		int rc = bind(sock, reinterpret_cast<sockaddr*>(&localaddr), addrlen);
		if (rc < 0)
		{
			int hr = WSAGetLastError();
			wprintf(L"PPP socket bind failed with error: %d\n", hr);
		}

		rc = ::connect(sock, reinterpret_cast<sockaddr*>(&remoteaddr), addrlen);
		if (rc != 0) {
			int hr = WSAGetLastError();
			wprintf(L"PPP socket connect failed with error: %d\n", hr);
		}

		closed_ = false;
		return 0;
	}

	void accept(const std::string& localHost, int localPort)
	{
		SOCKET local = socket(AF_INET, SOCK_STREAM, 0);

		resolveAddress(localHost, localPort, localaddr);

		// bind socket to local address.
		socklen_t addrlen = sizeof(sockaddr_in);
		int rc = ::bind(local, reinterpret_cast<sockaddr*>(&localaddr), addrlen);
		if (rc < 0)
		{
			int hr = WSAGetLastError();
			wprintf(L"PPP socket bind failed with error: %d\n", hr);
		}

		// start listening for incoming connection
		rc = ::listen(local, 1);
		if (rc < 0)
		{
			int hr = WSAGetLastError();
			wprintf(L"PPP socket listen failed with error: %d\n", hr);
		}

		// accept 1
		sock = ::accept(local, reinterpret_cast<sockaddr*>(&remoteaddr), &addrlen);
		if (sock == INVALID_SOCKET) {
			int hr = WSAGetLastError();
			wprintf(L"PPP accept failed with error: %d\n", hr);
		}

		closed_ = false;
	}

	// write to the serial port
	int write(const uint8_t* ptr, int count)
	{
		socklen_t addrlen = sizeof(sockaddr_in);
		int hr = send(sock, reinterpret_cast<const char*>(ptr), count, 0);
		if (hr == SOCKET_ERROR)
		{
			wprintf(L"PPP socket send failed with error: %d\n", hr);
		}

		return hr;
	}

	int read(uint8_t* result, int bytesToRead)
	{
		int bytesRead = 0;
		// try and receive something, up until port is closed anyway.

		while (!closed_)
		{
			socklen_t addrlen = sizeof(sockaddr_in);
			int rc = recv(sock, reinterpret_cast<char*>(result), bytesToRead, 0);
			if (rc < 0)
			{
#ifdef _WIN32
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


	void close()
	{
		if (!closed_) {
			closed_ = true;

#ifdef _WIN32
			closesocket(sock);
#else
			int fd = static_cast<int>(sock);
			::close(fd);
#endif
		}
	}

	std::string remoteAddress() {
		return inet_ntoa(remoteaddr.sin_addr);
	}

	int remotePort() {
		return ntohs(remoteaddr.sin_port);
	}

};

//-----------------------------------------------------------------------------------------

PPP::PPP()
{
	if (!socket_initialized_) {
		socket_initialized_ = true;
#ifdef _WIN32
		WSADATA wsaData;
		// Initialize Winsock
		int rc = WSAStartup(MAKEWORD(2, 2), (LPWSADATA)&wsaData);
		if (rc != 0) {
			wprintf(L"WSAStartup failed with error : %d\n", rc);
		}
#endif
	}
	impl_.reset(new PPPImpl());
}

PPP::~PPP()
{
	close();
}

void PPP::close()
{
	impl_->close();
}

void PPP::connect(const std::string& localHost, int localPort, const std::string& remoteHost, int remotePort)
{
	impl_->connect(localHost, localPort, remoteHost, remotePort);
}

void PPP::accept(const std::string& localHost, int localPort)
{
	impl_->accept(localHost, localPort);
}

int PPP::write(const uint8_t* ptr, int count)
{
	return impl_->write(ptr, count);
}

int
PPP::read(uint8_t* buffer, int bytesToRead)
{
	return impl_->read(buffer, bytesToRead);
}

bool PPP::isClosed()
{
	return impl_->isClosed();
}

std::string PPP::remoteAddress()
{
	return impl_->remoteAddress();
}

int PPP::remotePort()
{
	return impl_->remotePort();
}

int PPP::getRssi(const char* ifaceName)
{
    return impl_->getRssi(ifaceName);
}