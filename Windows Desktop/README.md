# Connecting The Nova on Windows

## WARNING WATCH YOUR DATA

### Currently you cannot set your connection to metered without editing the registry: https://answers.microsoft.com/en-us/windows/forum/windows_10-networking/how-to-set-a-dial-up-connection-to-metered-in/0b809279-c631-4641-bf12-170ec1d8a8b0?auth=1

### If you plan to use this on a windows 10 machine you should disable as many applications as you can from requesting connections. 

# Windows Desktop

In order to connect the nova to a PPP connection on windows we have to use the [Remote Access Service (RAS)](https://msdn.microsoft.com/en-us/library/windows/desktop/ms764032(v=vs.85).aspx) which has been a part of windows for years. Other modems might work with the [Mobile Broadband API (MBN API)](https://msdn.microsoft.com/en-us/library/windows/desktop/dd323269(v=vs.85).aspx) which is a newer method for interacting with these devices but they have to be recognized as a network adapter not as a modem, and so it is not compatible with the Nova.

The method for getting connected is the [RasDial](https://msdn.microsoft.com/en-us/library/windows/desktop/aa377004(v=vs.85).aspx) and then [RasHangUp](https://msdn.microsoft.com/en-us/library/windows/desktop/aa377567(v=vs.85).aspx). RasDial can be run asyncronously with a call back function that notifies the connection process and status. Use the documentation for more explanation but to get the status of the connection you will need the handle that is created by the RasDial function.

Before we can do that we need to create a connection profile. In the case of RAS this is stored in a phonebook since originally RAS was used for dialup connections. This setup is what will determine if the Nova will connect or not so making sure the correct flags and options are used during this process is paramount.

## Creating the Connection Profile

```
bool Modem::setupCellularConnection(LPWSTR modemName, LPWSTR connName)
{
    TCHAR tchNewEntry[MAX_PATH + 1] = TEXT("\0");
    wsprintf(tchNewEntry, connName);
    profileName = connName;

    // Validate the entry name
    if (RasValidateEntryName(NULL, tchNewEntry) != ERROR_ALREADY_EXISTS) {

        // Set up the RASENTRY structure. Use the country/area codes
        RASENTRY rasEntry;
        DWORD dwResult = 0;
        memset(&rasEntry, 0, sizeof(RASENTRY));

        rasEntry.dwSize = sizeof(RASENTRY);
        wsprintf(rasEntry.szDeviceName, modemName);
        wsprintf(rasEntry.szDeviceType, RASDT_Modem);
        wsprintf(rasEntry.szLocalPhoneNumber, TEXT("*99***1#"));
        rasEntry.dwFramingProtocol = RASFP_Ppp;
        rasEntry.dwType = RASET_Phone;
        rasEntry.dwfNetProtocols = RASNP_Ip;
        rasEntry.dwEncryptionType = ET_Optional;
        rasEntry.dwfOptions = RASEO_IpHeaderCompression|RASEO_RemoteDefaultGateway|RASEO_SwCompression|RASEO_ModemLights;
        rasEntry.dwfOptions2 = RASEO2_SecureFileAndPrint|RASEO2_SecureClientForMSNet|RASEO2_DontUseRasCredentials|RASEO2_Internet|RASEO2_DisableNbtOverIP|RASEO2_IPv6RemoteDefaultGateway;

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
        //get the current settings and make sure they are right here
    }
    connState = RASCS_Disconnected;
    return true;
}
```

The above method will create a new phone book entry for the nova. We have to first create a new/update an existing entry and then pass it along to the [RasSetEntryProperties](https://msdn.microsoft.com/en-us/library/windows/desktop/aa377827(v=vs.85).aspx) function. The actual important part of this is the flags and information set in the [RASENTRY](https://msdn.microsoft.com/en-us/library/windows/desktop/aa377274(v=vs.85).aspx) structure. Consult the documentation as the structure has a lot of options and flags that can be configured but the above configuration does allow the Nova to connect to the network.

`rasEntry.dwSize = sizeof(RASENTRY);`
This is just part of the documentation but it has to be done

`wsprintf(rasEntry.szDeviceName, modemName);`
This actually needs the name of the modem that will be connecting with and is an internal hardware name for the Nova. 

```
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
            wprintf(L"The following RAS Modems were found:\n");
            for (DWORD i = 0; i < dwDevices; i++) {
                if (wcscmp(lpRasDevInfo[i].szDeviceType, RASDT_Modem) == 0) {
                    wprintf(L"%s\n", lpRasDevInfo[i].szDeviceName);
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
```

This method will return all connected modems including the modem name that should be set for the szDeviceName variable. The structures contained in the returning vector will have a property also named szDeviceName

`wsprintf(rasEntry.szDeviceType, RASDT_Modem);`
Make sure the device type is set to a modem,

`wsprintf(rasEntry.szLocalPhoneNumber, TEXT("*99***1#"));`
This is the number the modem will dial when connecting, if it is not set here it can be sent as a parameter to the RasDial function and that is also ok.

`rasEntry.dwFramingProtocol = RASFP_Ppp;`
We are using the Point to Point Protocol 

`rasEntry.dwType = RASET_Phone;`
Careful with this, the other settings may return a successful connection but the network will not work. E.G. RASET_Broadband will connect but no network connection will be made.

`rasEntry.dwfNetProtocols = RASNP_Ip;`
We need at least ipv4 so set this flag to this

`rasEntry.dwEncryptionType = ET_Optional;`
This will attempt to use encryption if possible but no encryption will no make it fail. If you require encryption make sure your modem is able to connect after setting this to `ET_Require` or `ET_RequireMax`

`rasEntry.dwfOptions = RASEO_IpHeaderCompression|RASEO_RemoteDefaultGateway|RASEO_SwCompression|RASEO_ModemLights;`
These are flags that determine settings if you open up the network configurations. These were the flags after a manual setup but don't take this as 100% requirements

`rasEntry.dwfOptions2 = RASEO2_SecureFileAndPrint|RASEO2_SecureClientForMSNet|RASEO2_DontUseRasCredentials|RASEO2_Internet|RASEO2_DisableNbtOverIP|RASEO2_IPv6RemoteDefaultGateway;`
The options flag is so big there are two of them. Again not all of these might be required but they were part of the configuration that the manual setup had flagged. For a more detailed explanation of these flags consult the RASENTRY documentation.

## Connecting to the Network

```
bool Modem::connect()
{
    // Dial a RAS entry in synchronous mode
    hRasConn = NULL;
    RASDIALPARAMS rasDialParams;

    // Setup the RASDIALPARAMS structure for the entry we want
    // to dial
    memset(&rasDialParams, 0, sizeof(RASDIALPARAMS));

    rasDialParams.dwSize = sizeof(RASDIALPARAMS);
    wsprintf(rasDialParams.szEntryName, profileName);

    //auto retval = RasDial(NULL, NULL, &rasDialParams, 0L, (LPVOID)RasDialCallbackFunc, &hRasConn);
    auto retval = RasDial(NULL, NULL, &rasDialParams, 0L, NULL, &hRasConn);
    if (retval != SUCCESS) {
        wprintf(L"Encountered errer " + retval);
        return FALSE;
    }
    return true;
}
```

After creating the entry above connecting is simple, create a RASDIALPARAMS struct and tell it to use the connection profile we just created. Make sure its the same name as whatever was created before.

## Verifying the Connection Profile


You can check the profiles in multiple places on your PC. In the settings on Windows 10:
[Missing Quip Picture]
Or in the control panel:
[Missing Quip Picture]
If you right click and check the properties you will see what the flags we were setting actually did:


[Missing Quip Picture][Missing Quip Picture]
[Missing Quip Picture][Missing Quip Picture]
You can also right click or expand the connection and connect that way, just make sure to disconnect as well. Also be careful as windows will see this is a internet gateway so it will route traffic through the Nova if another connection is not available and will eat up a fair amount of data.

# Connecting via Serial 

[Missing Quip Picture]
This is the easy part, you can connect to the Nova as you would any other serial device. The Nova enumerates as multiple COM ports so make sure you connect to the correct one. Since you are probably connecting to get data and send AT commands you should be able to filter by the devices friendly names to connect to the right port.
The following will enumerate connected com ports

```
std::vector<SERIAL_DEVICE_INFO> Modem::getConnectedSerialDevices() {
    std::vector<SERIAL_DEVICE_INFO> devices;
    HDEVINFO hDevInfo = nullptr;
    SP_DEVINFO_DATA DeviceInterfaceData;
    DWORD dataType, actualSize = 0;

    // Search device set
    hDevInfo = SetupDiGetClassDevs((struct _GUID *)&GUID_SERENUM_BUS_ENUMERATOR, 0, 0, DIGCF_PRESENT);
    if (hDevInfo) {
        int i = 0;
        TCHAR dataBuf[MAX_PATH + 1];
        while (TRUE) {
            ZeroMemory(&DeviceInterfaceData, sizeof(DeviceInterfaceData));
            DeviceInterfaceData.cbSize = sizeof(DeviceInterfaceData);
            if (!SetupDiEnumDeviceInfo(hDevInfo, i, &DeviceInterfaceData)) {
                // SetupDiEnumDeviceInfo failed
                break;
            }

            if (SetupDiGetDeviceRegistryProperty(hDevInfo, &DeviceInterfaceData, SPDRP_FRIENDLYNAME,
                &dataType, (PBYTE)dataBuf, sizeof(dataBuf), &actualSize)) {

                TCHAR friendlyName[MAX_PATH + 1];
                TCHAR shortName[16];
                wsprintf(friendlyName, L"%s", dataBuf);

                // turn blahblahblah(COM4) into COM4

                TCHAR * begin = nullptr;
                TCHAR * end = nullptr;
                begin = wcsstr(dataBuf, L"COM");

                if (begin) {
                    end = wcsstr(begin, L")");
                    if (end) {
                        *end = 0;   // get rid of the )...
                        wcscpy_s(shortName, begin);
                    }
                }
                SERIAL_DEVICE_INFO device;
                device.portName = shortName;
                device.friendlyName = friendlyName;
                devices.push_back(device);
            }
            i++;
        }
    }
    SetupDiDestroyDeviceInfoList(hDevInfo);
    return devices;
}
```

And this is how you connect

```
bool Modem::setupSerialPort(std::wstring port, DWORD dwBaudrate)
{
    std::string strResult;
    DCB dcb = { 0 };

    if (m_hCom && m_hCom != INVALID_HANDLE_VALUE)
        CloseHandle(m_hCom);

    TCHAR pn[sizeof(port)];
    int num;
    if (swscanf_s(port.c_str(), L"COM%d", &num) == 1) {
        // Microsoft KB115831 a.k.a if COM > COM9 you have to use a different
        // syntax
        swprintf_s(pn, L"\\\\.\\COM%d", num);
    }
    else {
        wcsncpy_s(pn, (const TCHAR *)port.c_str(), sizeof(port) - 1);
    }

    m_hCom = CreateFile(pn, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
    if (m_hCom == INVALID_HANDLE_VALUE)
        return false;

    if (!GetCommState(m_hCom, &dcb))
        return false;

    dcb.BaudRate = dwBaudrate;

    if (!SetCommState(m_hCom, &dcb))
        return false;

    return true;
}
```

## Script Options

There are also optional scripts that can be run for a connection, and example can be found here: C:\Windows\System32\ras\pppmenu.scp
[pppmenu.scp](https://api.quip.com/2/blob/MEUAAAcsbOW/3JfsxhassUFOocNwTl8X9w?s=iRGbAZZlcJTz&name=pppmenu.scp) 

[Missing Quip Picture]
In order to manually set this up go to the network connections and edit the properties of the connection. On the security tab at the bottom there is a check box to run script and once it is selected you have to add in the path to the script in the text box that appears. 

Running custom connection scripts can be enabled in the RASENTRY structure discussed above and for information about the script see here:  http://www.itprotoday.com/management-mobility/windows-nt-ras-scripting
