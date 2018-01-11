#include <Windows.h>
#include <tchar.h>

#include <vector>

#include "MobileBroadband.h"

// Enable memory leak check for malloc and new
// Add this after all #include.
#ifdef _DEBUG
#define new DEBUG_NEW
#endif


MobileBroadbandManager::MobileBroadbandManager()
{
    HRESULT hr = S_OK;

    m_pManager = NULL;

    hr = CoCreateInstance(CLSID_MbnInterfaceManager, NULL, CLSCTX_SERVER, IID_IMbnInterfaceManager, (LPVOID*)&m_pManager);
    if (FAILED(hr))
        goto END;

END:
    return;
}

MobileBroadbandManager::~MobileBroadbandManager()
{
    SafeRelease(&m_pManager);
}

int MobileBroadbandManager::GetDeviceCount()
{
    int nCount = 0;
    HRESULT hr = S_OK;
    SAFEARRAY *psa = NULL;
    LONG lLower = 0;
    LONG lUpper = 0;

    if (!m_pManager)
        goto END;

    hr = m_pManager->GetInterfaces(&psa);
    if (FAILED(hr))
        goto END;

    hr = SafeArrayGetLBound(psa, 1, &lLower);
    if (FAILED(hr))
        goto END;

    hr = SafeArrayGetUBound(psa, 1, &lUpper);
    if (FAILED(hr))
        goto END;

    nCount = (int)(lUpper - lLower + 1);

END:
    SafeArrayDestroy(psa);
    return nCount;
} 

bool MobileBroadbandManager::GetPowerState(int nDeviceId, bool& fOn)
{
    bool fRet = false;
    HRESULT hr = S_OK;
    IMbnInterface* pInterface = NULL;
    IMbnRadio* pRadio = NULL;
    SAFEARRAY *psa = NULL;
    LONG lLower = 0;
    LONG lUpper = 0;
    MBN_READY_STATE readyState;
    MBN_RADIO radioState;
    ULONG ulRequestId = 0;

    if (!m_pManager)
        goto END;

    hr = m_pManager->GetInterfaces(&psa);
    if (FAILED(hr))
        goto END;

    for (LONG l = lLower; l <= lUpper; l++)
    {
        if (l != (LONG)nDeviceId)
            continue;

        hr = SafeArrayGetElement(psa, &l, (void*)(&pInterface));
        if (FAILED(hr))
            goto END;

        hr = pInterface->GetReadyState(&readyState);
        if (FAILED(hr))
            goto END;

        hr = pInterface->QueryInterface(IID_IMbnRadio, (void**)&pRadio);
        if (FAILED(hr))
            goto END;

        hr = pRadio->get_SoftwareRadioState(&radioState);
        if (FAILED(hr))
            goto END;

        fOn = (radioState == MBN_RADIO::MBN_RADIO_ON) ? true : false;

        SafeRelease(&pRadio);
        SafeRelease(&pInterface);

        fRet = true;

        break;
    }

END:
    SafeArrayDestroy(psa);
    SafeRelease(&pRadio);
    SafeRelease(&pInterface);

    return fRet;   
}

bool MobileBroadbandManager::SetPowerState(int nDeviceId, bool fOn)
{
    bool fRet = false;
    HRESULT hr = S_OK;
    IMbnInterface* pInterface = NULL;
    IMbnRadio* pRadio = NULL;
    SAFEARRAY *psa = NULL;
    LONG lLower = 0;
    LONG lUpper = 0;
    MBN_READY_STATE readyState;
    MBN_RADIO radioState;
    ULONG ulRequestId = 0;

    if (!m_pManager)
        goto END;

    hr = m_pManager->GetInterfaces(&psa);
    if (FAILED(hr))
        goto END;

    for (LONG l = lLower; l <= lUpper; l++)
    {
        if (l != (LONG)nDeviceId)
            continue;

        hr = SafeArrayGetElement(psa, &l, (void*)(&pInterface));
        if (FAILED(hr))
            goto END;

        hr = pInterface->GetReadyState(&readyState);
        if (FAILED(hr))
            goto END;

        hr = pInterface->QueryInterface(IID_IMbnRadio, (void**)&pRadio);
        if (FAILED(hr))
            goto END;

        radioState = (fOn) ? MBN_RADIO::MBN_RADIO_ON : MBN_RADIO::MBN_RADIO_OFF;
        hr = pRadio->SetSoftwareRadioState(radioState, &ulRequestId);
        if (FAILED(hr))
            goto END;

        SafeRelease(&pRadio);
        SafeRelease(&pInterface);

        fRet = true;

        break;
    }
END:
    SafeArrayDestroy(psa);
    SafeRelease(&pRadio);
    SafeRelease(&pInterface);

    return fRet;
}

bool MobileBroadbandManager::SetAllPowerState(bool fOn)
{
    bool fRet = false;
    HRESULT hr = S_OK;
    IMbnInterface* pInterface = NULL;
    IMbnRadio* pRadio = NULL;
    SAFEARRAY *psa = NULL;
    LONG lLower = 0;
    LONG lUpper = 0;
    MBN_READY_STATE readyState;
    MBN_RADIO radioState;
    ULONG ulRequestId = 0;

    if (!m_pManager)
        goto END;

    hr = m_pManager->GetInterfaces(&psa);
    if (FAILED(hr))
        goto END;

    for (LONG l = lLower; l <= lUpper; l++)
    {   
        hr = SafeArrayGetElement(psa, &l, (void*)(&pInterface));
        if (FAILED(hr))
            goto END;

        hr = pInterface->GetReadyState(&readyState);
        if (FAILED(hr))
            goto END;

        hr = pInterface->QueryInterface(IID_IMbnRadio, (void**)&pRadio);
        if (FAILED(hr))
            goto END;

        radioState = (fOn) ? MBN_RADIO::MBN_RADIO_ON : MBN_RADIO::MBN_RADIO_OFF;
        hr = pRadio->SetSoftwareRadioState(radioState, &ulRequestId);
        if (FAILED(hr))
            goto END;

        SafeRelease(&pRadio);
        SafeRelease(&pInterface);
    }

    fRet = true;

END:
    SafeArrayDestroy(psa);
    SafeRelease(&pRadio);
    SafeRelease(&pInterface);

    return fRet;
}

bool MobileBroadbandManager::GetSignalStrength(int nDeviceId, ULONG& ulStrength)
{
    bool fRet = false;
    HRESULT hr = S_OK;
    IMbnInterface* pInterface = NULL;
    IMbnSignal* pSignal = NULL;
    SAFEARRAY *psa = NULL;
    LONG lLower = 0;
    LONG lUpper = 0;
    MBN_READY_STATE readyState;

    if (!m_pManager)
        goto END;

    hr = m_pManager->GetInterfaces(&psa);
    if (FAILED(hr))
        goto END;

    for (LONG l = lLower; l <= lUpper; l++)
    {
        if (l != (LONG)nDeviceId)
            continue;

        hr = SafeArrayGetElement(psa, &l, (void*)(&pInterface));
        if (FAILED(hr))
            goto END;

        hr = pInterface->GetReadyState(&readyState);
        if (FAILED(hr))
            goto END;

        hr = pInterface->QueryInterface(IID_IMbnSignal, (void**)&pSignal);
        if (FAILED(hr))
            goto END;

        hr = pSignal->GetSignalStrength(&ulStrength);
        if (FAILED(hr))
            goto END;

        SafeRelease(&pSignal);
        SafeRelease(&pInterface);

        fRet = true;

        break;
    }
END:
    SafeArrayDestroy(psa);
    SafeRelease(&pSignal);
    SafeRelease(&pInterface);

    return fRet;
}