#pragma once
#include "EventBus/Event.h"
#include "../Network/Base/Network.h"

class ConnectionEvent : public Event
{
public:
	ConnectionEvent(NetworkType network) :
		network(network) {

	}
	~ConnectionEvent() {};

	NetworkType getNetwork() {
		return network;
	}

private:
	NetworkType network;
};

class DisconnectionEvent : public Event
{
public:
	DisconnectionEvent(NetworkType network) :
		network(network) {

	}
	~DisconnectionEvent() {};

	NetworkType getNetwork() {
		return network;
	}

private:
	NetworkType network;
};

class MessageSentEvent : public Event
{
public:
	MessageSentEvent(std::string message, bool source) :
		message(message),
		source(source) {

	}
	~MessageSentEvent() {}

	std::string getMessage() {
		return message;
	}

	bool wasSentViaATSocket() {
		return source;
	}

private:
	std::string message;
	bool source;
};

class MessageReceivedEvent : public Event
{
public:
	MessageReceivedEvent() {

	}
	~MessageReceivedEvent() {}
};

class SMSRecievedEvent : public Event
{
public:
	SMSRecievedEvent() {

	}

	~SMSRecievedEvent() {}
};

