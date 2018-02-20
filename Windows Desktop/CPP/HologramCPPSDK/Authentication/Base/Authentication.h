#pragma once
#include "../../Utils/Utils.h"
#include "JSON/json.hpp"
#include <map>

#define METADATA_VERSION "\x01";

using json = nlohmann::json;

class Authentication {
public:

	std::wstring buildPayloadString(std::wstring messages, std::vector<std::wstring> topics, std::string modemType = "", std::string modemId = "", std::string version = "") {
		buildAuthString();
		buildMetadataString(modemType, modemId, version); //type nova id sara

		if (topics.size() > 0) {
			buildTopicString(topics);
		}

		buildMessageString(messages);

		return StringToWstring(data.dump() + "\r\r");
	}


	virtual std::wstring buildSMSPayloadString(std::wstring message, std::string destination_number) =0;
	virtual bool supportsSMS() = 0;

protected:
	std::string buildModemTypeIdString(std::string modemType, std::string modemId) {
		if (modemType.empty()) {
			return "agnostic";
		}

		std::string payload = "";
		std::transform(modemType.begin(), modemType.end(), modemType.begin(), ::tolower);
		payload = modemType;

		if (modemType == "nova") {
			payload += ('-' + modemId);
		}

		return payload;
	}

	virtual void buildAuthString(std::string timestamp = "", std::string sequence_number = "") = 0;
	virtual void buildMetadataString(std::string modemType, std::string modemId, std::string version) = 0;
	virtual void buildTopicString(std::vector<std::wstring> topics) = 0;
	virtual void buildMessageString(std::wstring messages) = 0;

	std::map<std::string, std::string> credentials;
	json data;
};