#include "Nova_U201.h"

#define DEFAULT_NOVA_U201_TIMEOUT 200

Nova_U201::Nova_U201()
{
	name = "Nova_U201";
}

Nova_U201::~Nova_U201()
{
}

bool Nova_U201::createSocket()
{
	Modem::setupPDPContext();
	return Modem::createSocket();
}

void Nova_U201::setNetworkRegistrationStatus()
{
	sendATCommand("AT+CREG=2");
	sendATCommand("AT+CGREG=2");
}

void Nova_U201::initModemSerialMode()
{
	sendATCommand("ATE0");
	sendATCommand("AT+CMEE=2");
	sendATCommand("AT+CPIN?");
	setTimezoneConfiguration();
	sendATCommand("AT+CPMS=\"ME\",\"ME\",\"ME\"");
	setSMSConfiguration();
	setNetworkRegistrationStatus();
}

bool Nova_U201::isRegistered()
{
	return checkRegistered("AT+CREG") || checkRegistered("AT+CGREG");
}

void Nova_U201::handleURCSMS(std::string urcString)
{
}

void Nova_U201::handleURCLocation(std::string urcString)
{
}

void Nova_U201::handleURCListen(std::string urcString)
{
	std::vector<std::string> parts = ofSplitString(urcString.substr(urcString.find(":")), ",");

}

void Nova_U201::handleURC(std::string urcString)
{
	if (urcString.find("+CSIM:") != std::string::npos) {

	}
}
