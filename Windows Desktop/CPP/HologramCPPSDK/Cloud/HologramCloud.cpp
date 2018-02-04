#include "HologramCloud.h"

HologramCloud(std::map<std::string, std::string> credentials, bool enable_inbound, Network * network, Authentication * auth)
{
}

HologramCloud::~HologramCloud()
{
}

void HologramCloud::setAuthentication(Authentication * auth)
{
	this->auth = auth;
}
