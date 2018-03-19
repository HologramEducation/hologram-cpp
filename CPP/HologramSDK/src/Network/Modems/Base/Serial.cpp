#include "Serial.h"
#include <iostream>

bool Serial::setupSerialPort(std::wstring port, unsigned int baud)
{
#ifdef TARGET_WINDOWS
	std::string strResult;
	DCB dcb = { 0 };

	if (m_hCom && m_hCom != INVALID_HANDLE_VALUE)
		CloseHandle(m_hCom);

	m_hCom = CreateFile(port.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
	if (m_hCom == INVALID_HANDLE_VALUE)
		return false;

	if (!GetCommState(m_hCom, &dcb))
		return false;

	dcb.BaudRate = baud;

	if (!SetCommState(m_hCom, &dcb))
		return false;

	PurgeComm(m_hCom, PURGE_TXCLEAR | PURGE_TXABORT | PURGE_RXCLEAR | PURGE_RXABORT);
#else
    fd = ::open(fromWString(port).c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
	if (fd == -1) {
		return false;
	}

	struct termios options;
	tcgetattr(fd, &oldoptions);
	options = oldoptions;
	switch (baud) {
	case 300:
		cfsetispeed(&options, B300);
		cfsetospeed(&options, B300);
		break;
	case 1200:
		cfsetispeed(&options, B1200);
		cfsetospeed(&options, B1200);
		break;
	case 2400:
		cfsetispeed(&options, B2400);
		cfsetospeed(&options, B2400);
		break;
	case 4800:
		cfsetispeed(&options, B4800);
		cfsetospeed(&options, B4800);
		break;
	case 9600:
		cfsetispeed(&options, B9600);
		cfsetospeed(&options, B9600);
		break;
	case 14400:
		cfsetispeed(&options, B14400);
		cfsetospeed(&options, B14400);
		break;
	case 19200:
		cfsetispeed(&options, B19200);
		cfsetospeed(&options, B19200);
		break;
	case 28800:
		cfsetispeed(&options, B28800);
		cfsetospeed(&options, B28800);
		break;
	case 38400:
		cfsetispeed(&options, B38400);
		cfsetospeed(&options, B38400);
		break;
	case 57600:
		cfsetispeed(&options, B57600);
		cfsetospeed(&options, B57600);
		break;
	case 115200:
		cfsetispeed(&options, B115200);
		cfsetospeed(&options, B115200);
		break;
	case 230400:
		cfsetispeed(&options, B230400);
		cfsetospeed(&options, B230400);
		break;
	default:
		cfsetispeed(&options, B9600);
		cfsetospeed(&options, B9600);
		break;
	}

	options.c_cflag |= (CLOCAL | CREAD);
	options.c_cflag &= ~PARENB;
	options.c_cflag &= ~CSTOPB;
	options.c_cflag &= ~CSIZE;
	options.c_iflag &= (tcflag_t)~(INLCR | IGNCR | ICRNL | IGNBRK);
	options.c_oflag &= (tcflag_t)~(OPOST);
	options.c_cflag |= CS8;
#if defined( TARGET_LINUX )
	options.c_cflag |= CRTSCTS;
	options.c_lflag &= ~(ICANON | ECHO | ISIG);
#endif
	tcsetattr(fd, TCSANOW, &options);

	tcflush(fd, TCIOFLUSH);
	initialized = true;
#endif
	return true;
}

bool Serial::write(std::string message)
{
#ifdef TARGET_WINDOWS
	std::cout << message << std::endl;

	DWORD dwOut = 0;
	DWORD dwEvtMask = 0;

	if (!SetCommMask(m_hCom, EV_TXEMPTY)) {
		return false;
	}

	if (!WriteFile(m_hCom, message.data(), message.length(), &dwOut, NULL)) {
		return false;
	}

	WaitCommEvent(m_hCom, &dwEvtMask, NULL);  // Wait tx operation done.
#else
    ssize_t numWritten = ::write(fd, message.data(), message.length());
	if (numWritten <= 0) {
		if (errno == EAGAIN) {
			return false;
		}
		return false;
	}
#endif
	return true;
}

bool Serial::read(std::string & buffer, bool waitForBuffer)
{
#ifdef TARGET_WINDOWS
	DWORD dwOut = 0;
	DWORD dwEvtMask = 0;
	char* pBuffer = NULL;
	DWORD dwErrors = 0;
	COMSTAT comStat = { 0 };

	if (waitForBuffer) {
		if (!SetCommMask(m_hCom, EV_RXCHAR)) {
			return false;
		}

		WaitCommEvent(m_hCom, &dwEvtMask, NULL);  // Wait for the rx
	}

	if (!ClearCommError(m_hCom, &dwErrors, &comStat)) {
		return false;
	}

	if (!comStat.cbInQue) {
		return false;
	}

	pBuffer = new char[comStat.cbInQue + 1];
	if (!pBuffer) {
		return false;
	}

	memset(pBuffer, 0, comStat.cbInQue + 1);

	if (!ReadFile(m_hCom, pBuffer, comStat.cbInQue, &dwOut, NULL)) {
		if (pBuffer) {
			delete[] pBuffer;
			return false;
		}
	}

	buffer = pBuffer;
	std::cout << buffer << std::endl;
#else
	int length = 0;
	ioctl(fd, FIONREAD, &length);

	if (!length) {
		return false;
	}
    char buf[length+1];
    ssize_t nRead = ::read(fd, buf, length);
    buffer = buf;
	if (nRead < 0) {
		if (errno == EAGAIN)
			return false;
		return false;
	}
#endif

	return true;
}

void Serial::setTimeout(int timeout)
{
#ifdef TARGET_WINDOWS
	//technically this has no effect since this only happens during the read operation and thats not how
	//its been implemented, may want to refactor this and read until a new line character is reached
	COMMTIMEOUTS timeouts;
	GetCommTimeouts(m_hCom, &timeouts);
	timeouts.ReadIntervalTimeout = 50;
	timeouts.ReadTotalTimeoutConstant = timeout;
	timeouts.ReadTotalTimeoutMultiplier = 50;
	SetCommTimeouts(m_hCom, &timeouts);
#else
	//TODO: osx + linux timeouts
#endif
}

bool Serial::IsInitialized()
{
#ifdef TARGET_WINDOWS
	return (m_hCom && m_hCom != INVALID_HANDLE_VALUE) ? true : false;
#else
	return initialized;
#endif
    return false;
}

//https://stackoverflow.com/questions/7599331/list-usb-device-with-specified-vid-and-pid-without-using-windows-driver-kit
bool Serial::isDeviceConnected(SERIAL_DEVICE_INFO & info, std::wstring name)
{
	for (auto device : getConnectedSerialDevices()) {
		if (device.pid == info.pid && device.vid == info.vid && device.portName.find(name) != std::wstring::npos) {
			info.portName = device.portName;
			return true;
		}
	}
	return false;
}

//static functions

std::vector<SERIAL_DEVICE_INFO> Serial::getConnectedSerialDevices() {
	std::vector<SERIAL_DEVICE_INFO> devices;
#ifdef TARGET_WINDOWS
	WCHAR CurrentDevice[MAX_DEVICE_ID_LEN];
	ULONG length;
	CONFIGRET cr;
	std::vector<const GUID *> guids = { &GUID_DEVINTERFACE_COMPORT,  &GUID_DEVINTERFACE_MODEM };
	for (const GUID * guid : guids) {
		cr = CM_Get_Device_Interface_List_Size(
			&length,
			const_cast<GUID*>(guid),
			nullptr,        // pDeviceID
			CM_GET_DEVICE_INTERFACE_LIST_PRESENT);

		if ((cr != CR_SUCCESS) || (length == 0)) {
			wprintf(L"Failed to get interface list size of COM ports\n");
			continue;
		}

		std::vector<WCHAR> buf(length);
		cr = CM_Get_Device_Interface_List(
			const_cast<GUID*>(guid),
			nullptr,        // pDeviceID
			buf.data(),
			static_cast<ULONG>(buf.size()),
			CM_GET_DEVICE_INTERFACE_LIST_PRESENT);

		if ((cr != CR_SUCCESS) || (length == 0) || !buf[0]) {
			wprintf(L"Failed to get interface list of COM ports\n");
			continue;
		}

		*buf.rbegin() = UNICODE_NULL;

		ULONG index = 0;
		for (PCWSTR deviceInterface = buf.data();
			*deviceInterface;
			deviceInterface += wcslen(deviceInterface) + 1) {

			const DEVPROPKEY propkey = {
				PKEY_DeviceInterface_Serial_PortName.fmtid,
				PKEY_DeviceInterface_Serial_PortName.pid
			};
			DEVPROPTYPE propertyType;
			WCHAR portName[512];
			ULONG propertyBufferSize = sizeof(portName);
			cr = CM_Get_Device_Interface_Property(
				deviceInterface,
				&propkey,
				&propertyType,
				reinterpret_cast<BYTE*>(&portName),
				&propertyBufferSize,
				0); // ulFlags


			SERIAL_DEVICE_INFO info;
			info.portName = deviceInterface;
			parseVidPid(info.portName, info);

			propertyBufferSize = sizeof(CurrentDevice);
			cr = CM_Get_Device_Interface_Property(deviceInterface,
				&DEVPKEY_Device_InstanceId,
				&propertyType,
				(PBYTE)CurrentDevice,
				&propertyBufferSize,
				0);

			if (cr != CR_SUCCESS)
			{
				wprintf(L"Failed getting the current device\n");
			}

			else if (propertyType != DEVPROP_TYPE_STRING)
			{
				wprintf(L"Property is not string\n");
			}
			else {
				info.friendlyName.assign(GetDeviceDescription(CurrentDevice));
			}

			devices.push_back(info);
			++index;
		}
	}
#elif defined(TARGET_MACOS)
    kern_return_t kr;
    io_service_t device;
    IOCFPlugInInterface **plugInInterface = NULL;
    CFMutableDictionaryRef matchingDict;
    io_iterator_t iter;
    
    // Serial devices are instances of class IOSerialBSDClient.
    // Create a matching dictionary to find those instances.
    matchingDict = IOServiceMatching(kIOSerialBSDServiceValue);
    if (matchingDict == NULL) {
        printf("IOServiceMatching returned a NULL dictionary.\n");
    }
    else {
        // Look for devices that claim to be modems.
        CFDictionarySetValue(matchingDict,
                             CFSTR(kIOSerialBSDTypeKey),
                             CFSTR(kIOSerialBSDAllTypes));
        
        // Each serial device object has a property with key
        // kIOSerialBSDTypeKey and a value that is one of kIOSerialBSDAllTypes,
        // kIOSerialBSDModemType, or kIOSerialBSDRS232Type. You can experiment with the
        // matching by changing the last parameter in the above call to CFDictionarySetValue.
        
        // As shipped, this sample is only interested in modems,
        // so add this property to the CFDictionary we're matching on.
        // This will find devices that advertise themselves as modems,
        // such as built-in and USB modems. However, this match won't find serial modems.
    }
    
    // Get an iterator across all matching devices.
    kr = IOServiceGetMatchingServices(kIOMasterPortDefault, matchingDict, &iter);
    
    /* iterate */
    while ((device = IOIteratorNext(iter)))
    {
        SERIAL_DEVICE_INFO info;
        io_name_t deviceName;
        io_string_t path;
        CFTypeRef deviceNameAsCFString;
        CFTypeRef bsdPathAsCFString;
        UInt16 vendorId;
        UInt16 productId;
        
        // Get the callout device's path (/dev/cu.xxxxx). The callout device should almost always be
        // used: the dialin device (/dev/tty.xxxxx) would be used when monitoring a serial port for
        // incoming calls, e.g. a fax listener.
        
        bsdPathAsCFString = IORegistryEntryCreateCFProperty(device,
                                                            CFSTR(kIOCalloutDeviceKey),
                                                            kCFAllocatorDefault,
                                                            0);
        if (bsdPathAsCFString) {
            // Convert the path from a CFString to a C (NUL-terminated) string for use
            // with the POSIX open() call.
            
            Boolean result = CFStringGetCString((CFStringRef)bsdPathAsCFString, path, sizeof(path), kCFStringEncodingUTF8);
            
            if(result){
                info.portName = toWString(path);
            }
            
            CFRelease(bsdPathAsCFString);
        }
        
        io_name_t pathname;
        while(std::strncmp(pathname, "IOUSBInterface", 14) != 0){
            IOObjectGetClass(device, pathname);
            io_registry_entry_t parent;
            Boolean result = IORegistryEntryGetParentEntry(device, kIOServicePlane, &parent);
            if(result){
                break;
            }
            device = parent;
        }
        
        // Get the USB device's name.
        kr = IORegistryEntryGetName(device, deviceName);
        if(KERN_SUCCESS != kr) {
            deviceName[0] = '\0';
        }
        
        deviceNameAsCFString = CFStringCreateWithCString(kCFAllocatorDefault, deviceName, kCFStringEncodingASCII);
        
        
        if(deviceNameAsCFString) {
            Boolean result;
            
            // Convert from a CFString to a C (NUL-terminated)
            result = CFStringGetCString((CFStringRef)deviceNameAsCFString,
                                        deviceName,
                                        sizeof(deviceName),
                                        kCFStringEncodingUTF8);
            
            if(result) {
                info.friendlyName = toWString(deviceName);
            }
            
            CFRelease(deviceNameAsCFString);
        }
        // we need to create an IOUSBDeviceInterface for our device. This will create the necessary
        // connections between our userland application and the kernel object for the USB Device.
        SInt32 score;
        kr = IOCreatePlugInInterfaceForService(device, kIOUSBDeviceUserClientTypeID, kIOCFPlugInInterfaceID, &plugInInterface, &score);
        
        if((kIOReturnSuccess != kr) || !plugInInterface) {
            fprintf(stderr, "Failed getting plugin interface with error: 0x%08x.\n", kr);
            continue;
        }
        
        IOUSBDeviceInterface** deviceInterface;
        
        // Use the plugin interface to retrieve the device interface.
        HRESULT res = (*plugInInterface)->QueryInterface(plugInInterface, CFUUIDGetUUIDBytes(kIOUSBDeviceInterfaceID), (LPVOID*) &deviceInterface);
        
        // Now done with the plugin interface.
        (*plugInInterface)->Release(plugInInterface);
        
        if(res || deviceInterface == NULL) {
            fprintf(stderr, "QueryInterface returned %d.\n", (int) res);
            continue;
        }
        
        // Now that we have the IOUSBDeviceInterface, we can call the routines in IOUSBLib.h.        
        kr = (*deviceInterface)->GetDeviceVendor(deviceInterface, &vendorId);
        if(KERN_SUCCESS != kr) {
            fprintf(stderr, "GetDeviceVendor returned 0x%08x.\n", kr);
            continue;
        }
        info.vid = toWString(toHex(vendorId));
        
        kr = (*deviceInterface)->GetDeviceProduct(deviceInterface, &productId);
        if(KERN_SUCCESS != kr) {
            fprintf(stderr, "GetDeviceProduct returned 0x%08x.\n", kr);
            continue;
        }
        info.pid = toWString(toHex(productId));
        
        devices.push_back(info);
        
        /* And free the reference taken before continuing to the next item */
        IOObjectRelease(device);
    }
    
    /* Done, release the iterator */
    IOObjectRelease(iter);
#else
	std::vector<std::string> prefixMatch;

#ifdef TARGET_OSX
//we do this natively above but this also works
	prefixMatch.push_back("cu.");
	prefixMatch.push_back("tty.");

#endif

#ifdef TARGET_LINUX

	prefixMatch.push_back("ttyACM");
	prefixMatch.push_back("ttyS");
	prefixMatch.push_back("ttyUSB");
	prefixMatch.push_back("rfc");

#endif

	DIR *dir;
	dir = opendir("/dev");

	std::string deviceName = "";

	if (dir != nullptr) {
		//for each device
		struct dirent *entry;
		while ((entry = readdir(dir)) != nullptr) {
			deviceName = entry->d_name;
			//we go through the prefixes
			for (int k = 0; k < (int)prefixMatch.size(); k++) {
				//if the device name is longer than the prefix
				if (deviceName.size() > prefixMatch[k].size()) {
					//do they match ?
					if (deviceName.substr(0, prefixMatch[k].size()) == prefixMatch[k].c_str()) {
						SERIAL_DEVICE_INFO info;
						info.portName = StringToWstring("/dev/" + deviceName);
						parseVidPid(info.portName, info);
						devices.push_back(info);
						break;
					}
				}
			}
		}
		closedir(dir);
	}
#endif
	return devices;
}
