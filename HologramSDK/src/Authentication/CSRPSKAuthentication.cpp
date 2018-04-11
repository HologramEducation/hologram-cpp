#include "CSRPSKAuthentication.h"

CSRPSKAuthentication::CSRPSKAuthentication(std::map<std::string, std::string> credentials)
{
	this->credentials = credentials;
}

CSRPSKAuthentication::~CSRPSKAuthentication()
{
}

std::string CSRPSKAuthentication::buildPayloadString(std::string messages, std::vector<std::string> topics, std::string modemType, std::string modemId, std::string version)
{
	if (enforceValidDeviceKey()) {
		return Authentication::buildPayloadString(messages, topics, modemType, modemId, version);
	}
	return "";
}

std::string CSRPSKAuthentication::buildSMSPayloadString(std::string message, std::string destination_number)
{
	std::string send_data;

	if (enforceValidDeviceKey()) {
		send_data = "S" + credentials["devicekey"];
		send_data += destination_number + " " + message;
		send_data += "\r\r";
	}
	return send_data;
}

bool CSRPSKAuthentication::supportsSMS()
{
	return true;
}

void CSRPSKAuthentication::buildAuthString(std::string timestamp, std::string sequence_number)
{
	data["k"] = credentials["devicekey"];
}

void CSRPSKAuthentication::buildMetadataString(std::string modemType, std::string modemId, std::string version)
{
	data["m"] = "\x01" + buildModemTypeIdString(modemType, modemId) + "-" + version;
}

void CSRPSKAuthentication::buildTopicString(std::vector<std::string> topics)
{
	json topicArray = json::array();
	for (std::string topic : topics) {
		topicArray.push_back(topic);
	}
	data["t"] = topicArray;
}

void CSRPSKAuthentication::buildMessageString(std::string messages)
{
	data["d"] = messages;
}
