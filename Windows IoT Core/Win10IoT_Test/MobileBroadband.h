#pragma once

#include <mbnapi.h>

#include <vector>

// Release IXX Pointer.
template <class T> void SafeRelease(T** ppT)
{
	if (*ppT)
	{
		(*ppT)->Release();
		*ppT = NULL;
	}
}

// Need call CoInitialize() / CoUninitialize() for the process.
class MobileBroadbandManager {
public:
    MobileBroadbandManager();
    ~MobileBroadbandManager();

    int GetDeviceCount();

    bool GetPowerState(int nDeviceId, bool& fOn);
    bool SetPowerState(int nDeviceId, bool fOn);
    bool SetAllPowerState(bool fOn);

    bool GetSignalStrength(int nDeviceId, ULONG& ulStrength);

    IMbnInterfaceManager* m_pManager;
};