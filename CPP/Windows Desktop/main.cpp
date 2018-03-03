#include "../HologramSDK/src/Cloud/HologramCloud.h"
#include <iostream>

class EventListener : public EventHandler<MessageRecievedEvent>,  public EventHandler<MessageSentEvent>, public EventHandler<ConnectionEvent>
{
public:
	EventListener() { }

	virtual ~EventListener() { }


	/**
	* This event handler prints out a debug message whenever a message sent event is fired
	*
	* @param e The MessageRecievedEvent event
	*/
	virtual void onEvent(MessageRecievedEvent & e) override {

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

		std::wcout << "Message sent " << e.getMessage() << std::endl;
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
	EventBus::AddHandler<MessageRecievedEvent>(eventEcho);
	std::map<std::string, std::string> credentials;
	credentials.emplace("devicekey", argv[2]); //1 - 404 2 - 201
	HologramCloud cloud = HologramCloud(credentials, false, NetworkType::CELLULAR);
	std::vector<std::wstring> topics;
	topics.push_back(L"WINDOWS");
	topics.push_back(L"CPPSDK");
	std::string result = cloud.sendMessage(L"Windows Desktop", topics);
	auto code = cloud.parseResultString(result);
	//cloud.sendSMS(L"Just the CPP SDK sending SMSs eith the 404", argv[3]);
	return 0;
}