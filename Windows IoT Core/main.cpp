#include "../HologramSDK/src/Cloud/HologramCloud.h"
#include <iostream>

class EventListener : public EventHandler<MessageReceivedEvent>, public EventHandler<MessageSentEvent>, public EventHandler<ConnectionEvent>
{
public:
	EventListener() { }

	virtual ~EventListener() { }


	/**
	* This event handler prints out a debug message whenever a message sent event is fired
	*
	* @param e The MessageRecievedEvent event
	*/
	virtual void onEvent(MessageReceivedEvent & e) override {

		// Ignore the event if it's already been canceled
		if (e.getCanceled()) {
			return;
		}

		std::cout << "Message recieved" << std::endl;
	}


	/**
	* This event handler prints out a debug message whenever a message sent event is fired
	*
	* @param e The MessageSentEvent event
	*/
	virtual void onEvent(MessageSentEvent & e) override {

		// Ignore the event if it's already been canceled
		if (e.getCanceled()) {
			return;
		}

		std::cout << "Message sent " << e.getMessage() << std::endl;
	}

	/**
	* This event handler prints out a debug message whenever a connection event is fired
	*
	* @param e The ConnectionEvent event
	*/
	virtual void onEvent(ConnectionEvent & e) override {

		// Ignore the event if it's already been canceled
		if (e.getCanceled()) {
			return;
		}

		std::cout << "Connected to network" << std::endl;
	}
};

int main(int argc, char* argv[])
{
	EventListener eventEcho;
	EventBus::AddHandler<MessageSentEvent>(eventEcho);
	EventBus::AddHandler<ConnectionEvent>(eventEcho);
	EventBus::AddHandler<MessageReceivedEvent>(eventEcho);
	std::map<std::string, std::string> credentials;
	credentials.emplace("devicekey", argv[1]); //1 - 404 2 - 201
	HologramCloud cloud = HologramCloud(credentials, false, NetworkType::CELLULAR);
	std::vector<std::string> topics;
	topics.push_back("WINDOWSIOT");
	topics.push_back("CPPSDK");
	std::string result = cloud.sendMessage("Windows IoT Core", topics);
	auto code = cloud.parseResultString(result);
	//cloud.sendSMS(L"Just the CPP SDK sending SMSs eith the 404", argv[3]);
	return 0;
}