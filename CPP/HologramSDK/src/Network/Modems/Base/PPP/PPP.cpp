// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "PPP.h"

class PPP::PPPImpl
{
#ifdef TARGET_WINDOWS
	SOCKET sock = INVALID_SOCKET;
#else
    int sock = 0;
#endif
	sockaddr_in localaddr;
	sockaddr_in remoteaddr;
	bool closed_ = true;
public:

    PPPImpl(){
#ifdef USERAS
        hRasConn = NULL;
#endif
    }
    
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
#ifdef USERAS
        // Dial a RAS entry in synchronous mode
        hRasConn = NULL;
        RASDIALPARAMS rasDialParams;
        
        // Setup the RASDIALPARAMS structure for the entry we want
        // to dial
        memset(&rasDialParams, 0, sizeof(RASDIALPARAMS));
        
        rasDialParams.dwSize = sizeof(RASDIALPARAMS);
        wsprintf(rasDialParams.szEntryName, profileName.c_str());
        
        auto retval = RasDial(NULL, NULL, &rasDialParams, 0L, NULL, &hRasConn);
        if (retval != SUCCESS) {
            wprintf(L"Encountered errer " + retval);
            return 0;
        }
#else
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
			wprintf(L"PPP socket bind failed with error: %d\n", hr);
#endif
		}

		rc = ::connect(sock, reinterpret_cast<sockaddr*>(&remoteaddr), addrlen);
		if (rc != 0) {
#ifdef TARGET_WINDOWS
			int hr = WSAGetLastError();
			wprintf(L"PPP socket connect failed with error: %d\n", hr);
#endif
		}

		closed_ = false;
		return 0;
#endif
	}

	void accept(const std::string& localHost, int localPort)
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
			wprintf(L"PPP socket bind failed with error: %d\n", hr);
#endif
		}

		// start listening for incoming connection
		rc = ::listen(local, 1);
		if (rc < 0)
		{
#ifdef TARGET_WINDOWS

			int hr = WSAGetLastError();
			wprintf(L"PPP socket listen failed with error: %d\n", hr);
#endif
		}

		// accept 1
		sock = ::accept(local, reinterpret_cast<sockaddr*>(&remoteaddr), &addrlen);
		if (sock == INVALID_SOCKET) {
#ifdef TARGET_WINDOWS
			int hr = WSAGetLastError();
			wprintf(L"PPP accept failed with error: %d\n", hr);
#endif
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


	void close()
	{
		if (!closed_) {
			closed_ = true;

#ifdef TARGET_WINDOWS
#ifdef USERAS
            if (hRasConn != NULL) {
                RasHangUp(hRasConn);
            }
#else
			closesocket(sock);
#endif
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
#ifdef USERAS
    bool Modem::setupRASConnection(std::wstring modemName, std::wstring connName)
    {
        wchar_t tchNewEntry[MAX_PATH + 1] = TEXT("\0");
        wsprintf(tchNewEntry, connName.c_str());
        profileName = connName;
        
        // Validate the entry name
        if (RasValidateEntryName(NULL, tchNewEntry) != ERROR_ALREADY_EXISTS) {
            
            // Set up the RASENTRY structure
            RASENTRY rasEntry;
            DWORD dwResult = 0;
            memset(&rasEntry, 0, sizeof(RASENTRY));
            
            rasEntry.dwSize = sizeof(RASENTRY);
            wsprintf(rasEntry.szDeviceName, modemName.c_str());
            wsprintf(rasEntry.szDeviceType, RASDT_Modem);
            wsprintf(rasEntry.szLocalPhoneNumber, TEXT("*99***1#"));
            rasEntry.dwFramingProtocol = RASFP_Ppp;
            rasEntry.dwType = RASET_Phone;
            rasEntry.dwfNetProtocols = RASNP_Ip;
            rasEntry.dwEncryptionType = ET_Optional;
            rasEntry.dwfOptions = RASEO_IpHeaderCompression | RASEO_RemoteDefaultGateway | RASEO_SwCompression | RASEO_ModemLights;
            rasEntry.dwfOptions2 = RASEO2_SecureFileAndPrint | RASEO2_SecureClientForMSNet | RASEO2_DontUseRasCredentials | RASEO2_Internet | RASEO2_DisableNbtOverIP | RASEO2_IPv6RemoteDefaultGateway;
            
            // Create the entry
            dwResult = RasSetEntryProperties(NULL, tchNewEntry, &rasEntry,
                                             sizeof(RASENTRY), NULL, 0);
            
            // Check for any errors
            if (dwResult != 0) {
                TCHAR tchError[256] = TEXT("\0");
                
                // Print out the error
                wsprintf(tchError, TEXT("Could not create entry -- Error %ld"),
                         dwResult);
                return false;
            }
        }
        else {
            // update the current settings and make sure they are right
            // Set up the RASENTRY structure
            RASENTRY rasEntry;
            DWORD dwResult = 0;
            memset(&rasEntry, 0, sizeof(RASENTRY));
            
            rasEntry.dwSize = sizeof(RASENTRY);
            wsprintf(rasEntry.szDeviceName, modemName.c_str());
            wsprintf(rasEntry.szDeviceType, RASDT_Modem);
            wsprintf(rasEntry.szLocalPhoneNumber, TEXT("*99***1#"));
            rasEntry.dwFramingProtocol = RASFP_Ppp;
            rasEntry.dwType = RASET_Phone;
            rasEntry.dwfNetProtocols = RASNP_Ip;
            rasEntry.dwEncryptionType = ET_Optional;
            rasEntry.dwfOptions = RASEO_IpHeaderCompression | RASEO_RemoteDefaultGateway | RASEO_SwCompression | RASEO_ModemLights;
            rasEntry.dwfOptions2 = RASEO2_SecureFileAndPrint | RASEO2_SecureClientForMSNet | RASEO2_DontUseRasCredentials | RASEO2_Internet | RASEO2_DisableNbtOverIP | RASEO2_IPv6RemoteDefaultGateway;
            
            // Create the entry
            dwResult = RasSetEntryProperties(NULL, tchNewEntry, &rasEntry,
                                             sizeof(RASENTRY), NULL, 0);
            
            // Check for any errors
            if (dwResult != 0) {
                TCHAR tchError[256] = TEXT("\0");
                
                // Print out the error
                wsprintf(tchError, TEXT("Could not create entry -- Error %ld"),
                         dwResult);
                return false;
            }
        }
        connState = RASCS_Disconnected;
        return true;
    }
    
    std::vector<LPRASDEVINFO> Modem::getConnectedModems() {
        
        DWORD dwCb = 0;
        DWORD dwRet = ERROR_SUCCESS;
        DWORD dwDevices = 0;
        LPRASDEVINFO lpRasDevInfo = NULL;
        std::vector<LPRASDEVINFO> devices;
        
        // Call RasEnumDevices with lpRasDevInfo = NULL. dwCb is returned with the required buffer size and
        // a return code of ERROR_BUFFER_TOO_SMALL
        dwRet = RasEnumDevices(lpRasDevInfo, &dwCb, &dwDevices);
        
        if (dwRet == ERROR_BUFFER_TOO_SMALL) {
            // Allocate the memory needed for the array of RAS structure(s).
            lpRasDevInfo = (LPRASDEVINFO)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwCb);
            if (lpRasDevInfo == NULL) {
                wprintf(L"HeapAlloc failed!\n");
                return devices;
            }
            // The first RASDEVINFO structure in the array must contain the structure size
            lpRasDevInfo[0].dwSize = sizeof(RASDEVINFO);
            
            // Call RasEnumDevices to enumerate RAS devices
            dwRet = RasEnumDevices(lpRasDevInfo, &dwCb, &dwDevices);
            
            // If successful, print the names of the RAS devices
            if (ERROR_SUCCESS == dwRet) {
                for (unsigned int i = 0; i < dwDevices; i++) {
                    if (wcscmp(lpRasDevInfo[i].szDeviceType, RASDT_Modem) == 0) {
                        devices.push_back(&lpRasDevInfo[i]);
                    }
                }
            }
            //Deallocate memory for the connection buffer
            HeapFree(GetProcessHeap(), 0, lpRasDevInfo);
            lpRasDevInfo = NULL;
            return devices;
        }
        
        // There was either a problem with RAS or there are no RAS devices to enumerate
        if (dwDevices >= 1) {
            wprintf(L"The operation failed to acquire the buffer size.\n");
        }
        else {
            wprintf(L"There were no RAS devices found.\n");
        }
        
        return devices;
    }
    
    void Modem::getConnectionProfiles()
    {
        DWORD dwErr = ERROR_SUCCESS;
        DWORD dwSize = 0, dwEntries;
        LPRASENTRYNAME pRasEntryName = NULL;
        LPRASENTRY pRasEntry = NULL;
        unsigned int i;
        
        dwErr = RasEnumEntries(NULL, NULL, NULL, &dwSize, &dwEntries);
        
        if (dwErr == ERROR_BUFFER_TOO_SMALL)
        {
            pRasEntryName = (LPRASENTRYNAME)LocalAlloc(LPTR, dwSize);
            if (pRasEntryName == NULL)
            {
                printf("Alloc failed for EnumEntries %d", GetLastError());
                goto Done;
            }
            
            pRasEntryName->dwSize = sizeof(RASENTRYNAME);
            dwErr = RasEnumEntries(NULL, NULL, pRasEntryName, &dwSize, &dwEntries);
            if (dwErr != ERROR_SUCCESS)
            {
                printf("RasEnumEntries failed with error %d", dwErr);
                goto Done;
            }
            printf("Number of Entries %d\n", dwEntries);
            
            for (i = 0; i < dwEntries; i++)
            {
                dwSize = 0;
                dwErr = RasGetEntryProperties(pRasEntryName[i].szPhonebookPath,
                                              pRasEntryName[i].szEntryName,
                                              NULL,
                                              &dwSize,
                                              NULL, NULL);
                if (dwErr == ERROR_BUFFER_TOO_SMALL)
                {
                    pRasEntry = (LPRASENTRY)LocalAlloc(LPTR, dwSize);
                    if (pRasEntry == NULL)
                    {
                        printf("Alloc failed for RasEntry\n");
                        goto Done;
                    }
                    
                    pRasEntry->dwSize = sizeof(RASENTRY);
                    dwErr = RasGetEntryProperties(pRasEntryName[i].szPhonebookPath,
                                                  pRasEntryName[i].szEntryName,
                                                  pRasEntry,
                                                  &dwSize,
                                                  NULL, 0);
                    if (dwErr != ERROR_SUCCESS)
                    {
                        wprintf(L"RasGetEntryProperties failed with error %d for entry %s\n", dwErr, pRasEntryName[i].szEntryName);
                        goto Done;
                    }
                }
                else
                {
                    wprintf(L"RasGetEntryProperties failed with error %d for entry %s\n", dwErr, pRasEntryName[i].szEntryName);
                    goto Done;
                }
                determineOFlags(pRasEntry->dwfOptions);
                determineO2Flags(pRasEntry->dwfOptions2);
                LocalFree(pRasEntry);
                pRasEntry = NULL;
            }
        }
        else
        {
            goto Done;
        }
        
    Done:
        if (pRasEntryName)
        {
            LocalFree(pRasEntryName);
        }
        if (pRasEntry)
        {
            LocalFree(pRasEntry);
        }
    }
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
#endif
};

//-----------------------------------------------------------------------------------------

PPP::PPP()
{
	if (!socket_initialized_) {
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
