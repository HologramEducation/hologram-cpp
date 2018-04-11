#include "Nova_R404.h"

#define DEFAULT_NOVA_R404_TIMEOUT 200

SERIAL_DEVICE_INFO Nova_R404::deviceInfo = SERIAL_DEVICE_INFO("05C6", "90B2");

Nova_R404::Nova_R404()
{
	name = "Nova_R404";
}

Nova_R404::~Nova_R404()
{
}

std::string Nova_R404::sendMessage(std::string message)
{
	urcState = SOCKET_INIT;

	writeSocket(message);

	while (urcState != SOCKET_SEND_READ && urcState != SOCKET_CLOSED) {
		checkURC();
		write("");
        std::chrono::milliseconds timespan(RETRY_DELAY);
        std::this_thread::sleep_for(timespan);
	}
	if (urcState == SOCKET_SEND_READ) {
		EventBus::FireEvent(MessageReceivedEvent());
		return readSocket(socketId, last_read_payload_length);
	}
	else {
		return "";
	}
}

void Nova_R404::setNetworkRegistrationStatus()
{
	sendATCommand("AT+CEREG=2");
}

void Nova_R404::initModemSerialMode()
{
	sendATCommand("ATE0");
	sendATCommand("AT+CMEE=2");
	sendATCommand("AT+CPIN?");
	//setTimezoneConfiguration();
	sendATCommand("AT+CPMS=\"ME\",\"ME\",\"ME\"");
	setSMSConfiguration();
	setNetworkRegistrationStatus();
}

bool Nova_R404::isRegistered()
{
	return checkRegistered("AT+CEREG?");
}

void Nova_R404::handleURCSMS(std::string urcString)
{
}

void Nova_R404::handleURCLocation(std::string urcString)
{
}

void Nova_R404::handleURCListen(std::string urcString)
{
	std::vector<std::string> parts = ofSplitString(urcString.substr(urcString.find(":")), ",");

}

void Nova_R404::handleURC(std::string urcString)
{
	if (urcString.find("+CSIM:") != std::string::npos) {

	}
	Modem::handleURC(urcString);
}

void Nova_R404::populateModemInformation()
{
	std::vector<std::string> result;
	if (sendAndParseATCommand("ATI", result)) {
		for (auto entry : result)
		{
			if (entry.find("Manufacturer: ") != std::string::npos)
				modemInfo.Manufacturer = entry.substr(strlen("Manufacturer: "));
			else if (entry.find("Model: ") != std::string::npos)
				modemInfo.Model = entry.substr(strlen("Model: "));
			else if (entry.find("Revision: ") != std::string::npos)
				modemInfo.Revision = entry.substr(strlen("Revision: "));
			else if (entry.find("SVN: ") != std::string::npos)
				modemInfo.SVN = entry.substr(strlen("SVN: "));
			else if (entry.find("IMEI: ") != std::string::npos)
				modemInfo.IMEI = entry.substr(strlen("IMEI: "));
		}
	}

	if (sendAndParseATCommand("AT+CCID?", result) == MODEM_OK) {
		modemInfo.ICCID = result[0];
	}
}
