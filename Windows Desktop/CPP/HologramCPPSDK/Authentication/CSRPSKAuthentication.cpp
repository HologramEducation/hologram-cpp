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
	if (!enforceValidDeviceKey()) {
		Authentication::buildPayloadString(messages, topics, modemType, modemId, version);
	}
	return StringToWstring(data.dump() + "\r\r");
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
	data["m"] = METADATA_VERSION;
	data["m"] += buildModemTypeIdString(modemType, modemId) + "-" + version;
}

void CSRPSKAuthentication::buildTopicString(std::vector<std::wstring> topics)
{
	data["t"] = topics;
}

void CSRPSKAuthentication::buildMessageString(std::wstring messages)
{
	data["d"] = messages;
}
