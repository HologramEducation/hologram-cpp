#include "CSRPSKAuthentication.h"

CSRPSKAuthentication::CSRPSKAuthentication(std::map<std::string, std::string> credentials)
{
	this->credentials = credentials;
}

CSRPSKAuthentication::~CSRPSKAuthentication()
{
}

std::wstring CSRPSKAuthentication::buildPayloadString(std::wstring messages, std::vector<std::wstring> topics, std::string modemType, std::string modemId, std::string version)
{
	if (enforceValidDeviceKey()) {
		return Authentication::buildPayloadString(messages, topics, modemType, modemId, version);
	}
	return L"";
}

std::wstring CSRPSKAuthentication::buildSMSPayloadString(std::string destination_number, std::wstring message)
{
	std::wstring send_data;

	if (!enforceValidDeviceKey()) {
		send_data = L"S" + StringToWstring(credentials["devicekey"]);
		send_data += StringToWstring(destination_number + " ") + message;
		send_data += L"\r\r";
	}
	return send_data;
}

void CSRPSKAuthentication::buildAuthString(std::string timestamp, std::string sequence_number)
{
	data["k"] = credentials["devicekey"];
}

void CSRPSKAuthentication::buildMetadataString(std::string modemType, std::string modemId, std::string version)
{
	data["m"] = "\x01" + buildModemTypeIdString(modemType, modemId) + "-" + version;
}

void CSRPSKAuthentication::buildTopicString(std::vector<std::wstring> topics)
{
	json topicArray = json::array();
	for (std::wstring topic : topics) {
		topicArray.push_back(WstringToString(topic));
	}
	data["t"] = topicArray;
}

void CSRPSKAuthentication::buildMessageString(std::wstring messages)
{
	data["d"] = WstringToString(messages);
}
