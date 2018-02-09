#include "HologramCloud.h"
#include "../Authentication/CSRPSKAuthentication.h"

HologramCloud::HologramCloud(std::map<std::string, std::string> credentials, bool enable_inbound, NetworkType type, Authentication * auth) : 
	CustomCloud(HOLOGRAM_HOST_SEND, HOLOGRAM_PORT_SEND, HOLOGRAM_HOST_RECEIVE, HOLOGRAM_PORT_RECEIVE)
{
	if (auth == NULL) {
		this->authenticator = new CSRPSKAuthentication(credentials);
	}
	else {
		this->authenticator = auth;
	}
	this->networkManager.setNetworkType(type);
}

HologramCloud::~HologramCloud()
{
}

void HologramCloud::setAuthentication(Authentication * auth)
{
	this->authenticator = auth;
}

std::string HologramCloud::sendMessage(std::wstring message, std::vector<std::wstring> topics)
{
	if (!isReadyToSend()) {
		addPayloadToBuffer(message);
		return std::string();
	}

	std::wstring encodedMessage;
	if (networkManager.getNetworkType() == CELLULAR) {
		Modem * modem = ((Cellular *) networkManager.getNetwork())->modem;
		encodedMessage = authenticator->buildPayloadString(message, topics, modem->getName(), WstringToString(modem->modemInfo.Model), VERSION);
	}
	else {
		encodedMessage = authenticator->buildPayloadString(message, topics, "", "", VERSION);
	}

	std::string result = CustomCloud::sendMessage(encodedMessage);
	return result;
}

void HologramCloud::sendSMS(std::wstring message, std::string destNumber)
{
	if (authenticator->supportsSMS() && message.length() <= MAX_SMS_LENGTH && destNumber[0] == '+') {
		std::wstring encodedMessage = authenticator->buildSMSPayloadString(message, destNumber);
		std::string result = CustomCloud::sendMessage(encodedMessage);
	}
}
