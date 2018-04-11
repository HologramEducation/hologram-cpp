#include "SCN.h"

SCN::SCN(std::string name, Serial * serialport)
{
	this->serialport = serialport;
    serviceId = CFStringCreateWithCString( NULL, "Hologram PPP Connection" , kCFStringEncodingUTF8 );
}

SCN::~SCN()
{
    if (m_connection)
    {
        SCNetworkConnectionUnscheduleFromRunLoop(m_connection, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
        CFRelease(m_connection);
        m_connection = NULL;
    }
}

bool SCN::connect(){
    const void *pppKeys[3], *pppVals[3];
    pppKeys[index] = (void*) kSCPropNetPPPAuthName;
    pppVals[index] = (void*) cfUsername;
    
    CFDictionaryRef pppDialOptions;
    pppDialOptions = CFDictionaryCreate( NULL
                                        , pppKeys
                                        , pppVals
                                        , index
                                        , &kCFTypeDictionaryKeyCallBacks
                                        , &kCFTypeDictionaryValueCallBacks );
    
    CFStringRef     keys[] = { kSCEntNetPPP   };
    CFDictionaryRef vals[] = { pppDialOptions };
    
    CFDictionaryRef optionsForDial;
    optionsForDial = CFDictionaryCreate( NULL
                                        , (const void **) &keys
                                        , (const void **) &vals
                                        , 1
                                        , &kCFTypeDictionaryKeyCallBacks
                                        , &kCFTypeDictionaryValueCallBacks );
    
    // create a connection reference
    if (m_connection)
    {
        SCNetworkConnectionUnscheduleFromRunLoop(m_connection, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
        CFRelease(m_connection);
        m_connection = NULL;
    }
    
    for (int I = 0; I < 3; I++)
    {
        SCNetworkConnectionRef connection;
        connection = SCNetworkConnectionCreateWithServiceID(NULL, serviceId, Connection::callback, NULL );
        if (!connection) return NULL;
        
        Boolean ok;
        ok = SCNetworkConnectionScheduleWithRunLoop(connection, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode );
        if (!ok) return NULL;
        
        ok = SCNetworkConnectionStart(connection, optionsForDial, false);
        if (ok) return connection;
        
        sleep(3);
    }
}
