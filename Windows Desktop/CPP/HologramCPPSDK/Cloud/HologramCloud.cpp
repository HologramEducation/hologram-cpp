#include "HologramCloud.h"

HologramCloud::HologramCloud(std::map<std::string, std::string> credentials, bool enable_inbound, Network * network, Authentication * auth) : 
	CustomCloud(HOLOGRAM_HOST_SEND, HOLOGRAM_PORT_SEND, HOLOGRAM_HOST_RECEIVE, HOLOGRAM_PORT_RECEIVE)
{
}

HologramCloud::~HologramCloud()
{
}

void HologramCloud::setAuthentication(Authentication * auth)
{
	this->auth = auth;
}

std::string HologramCloud::sendMessage(std::wstring message)
{

	return std::string();
}
