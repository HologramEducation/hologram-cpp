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
	fd = open(port.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
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
	int numWritten = write(fd, message.data(), message.length());
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
	int nRead = read(fd, buffer, length);
	if (nRead < 0) {
		if (errno == EAGAIN)
			return false;
		return false;
	}
	return nRead;
#endif

	return true;
}

void Serial::setTimeout(int timeout)
{
#ifdef TARGET_WINDOWS
	COMMTIMEOUTS timeouts;
	GetCommTimeouts(m_hCom, &timeouts);
	timeouts.ReadIntervalTimeout = 50;
	timeouts.ReadTotalTimeoutConstant = timeout;
	timeouts.ReadTotalTimeoutMultiplier = 50;
	SetCommTimeouts(m_hCom, &timeouts);
#else
	tcsetattr(fd, TCSANOW, &options);
#endif
}

bool Serial::IsInitialized()
{
#ifdef TARGET_WINDOWS
	return (m_hCom && m_hCom != INVALID_HANDLE_VALUE) ? true : false;
#endif
    return false;
}

//https://stackoverflow.com/questions/7599331/list-usb-device-with-specified-vid-and-pid-without-using-windows-driver-kit
bool Serial::isDeviceConnected(SERIAL_DEVICE_INFO & info, std::wstring name)
{
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
			return false;
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
			return false;
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

			if (isCorrectDevice(deviceInterface, info, name))
			{
				info.portName = deviceInterface;

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
				return true;
			}
			++index;
		}
	}
#else
	for (auto device : getConnectedSerialDevices()) {
		if (device.pid == info.pid && device.vid == info.vid) {
			
		}
	}
#endif
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
#else
	std::vector<std::string> prefixMatch;

#ifdef TARGET_OSX

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
		int deviceCount = 0;
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
						deviceCount++;
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
