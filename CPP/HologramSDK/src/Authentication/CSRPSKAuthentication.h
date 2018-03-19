#pragma once
#include "Base/Authentication.h"

#define DEVICE_KEY_LEN  8

class CSRPSKAuthentication : public Authentication
{
public:
	CSRPSKAuthentication(std::map<std::string, std::string> credentials);
	~CSRPSKAuthentication();

	std::string buildPayloadString(std::string message, std::vector<std::string> topics, std::string modemType = "", std::string modemId = "", std::string version = "");
	virtual std::string buildSMSPayloadString(std::string message, std::string destination_number);
	virtual bool supportsSMS();

private:
	virtual void buildAuthString(std::string timestamp = "", std::string sequence_number = "");
	virtual void buildMetadataString(std::string modemType, std::string modemId, std::string version);
	virtual void buildTopicString(std::vector<std::string> topics);
	virtual void buildMessageString(std::string messages);

	bool enforceValidDeviceKey() {
		if (credentials.find("devicekey") == credentials.end()) {
			return false;
		}
		if (credentials["devicekey"].size() != DEVICE_KEY_LEN) {
			return false;
		}
		return true;
	}
};

