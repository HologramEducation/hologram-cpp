#pragma once
#include "CustomCloud.h"
#include "../Authentication/Base/Authentication.h"

#define DEFAULT_SEND_MESSAGE_TIMEOUT 5
#define HOLOGRAM_HOST_SEND "cloudsocket.hologram.io"
#define HOLOGRAM_PORT_SEND 9999
#define HOLOGRAM_HOST_RECEIVE "0.0.0.0"
#define HOLOGRAM_PORT_RECEIVE 4010
#define MAX_SMS_LENGTH 160

enum HOLOGRAM_ERROR_CODES {
	ERR_OK,
	ERR_CONNCLOSED, // Connection was closed so we couldn't read enough
	ERR_MSGINVALID, // Couldn't parse the message
	ERR_AUTHINVALID, // Auth section of message was invalid
	ERR_PAYLOADINVALID,//# Payload type was invalid
	ERR_PROTINVALID, // Protocol type was invalid
	ERR_INTERNAL, // An internal error occurred
	ERR_METADATA, // Metadata was formatted incorrectly
	ERR_TOPICINVALID, // Topic was formatted incorrectly
	ERR_NOTCONNECTED, // The Network is not connected
	ERR_UNKNOWN = -1// Unknown error
};

class HologramCloud : public CustomCloud
{
public:
	HologramCloud(std::map<std::string, std::string> credentials, bool enable_inbound = false, NetworkType type = CELLULAR, Authentication * auth = NULL);
	~HologramCloud();
	void setAuthentication(Authentication * auth);
	virtual void sendSMS(std::wstring message, std::string destNumber);
	virtual std::string sendMessage(std::wstring message, std::vector<std::wstring> topics);
	HOLOGRAM_ERROR_CODES parseResultString(std::string result);
};

