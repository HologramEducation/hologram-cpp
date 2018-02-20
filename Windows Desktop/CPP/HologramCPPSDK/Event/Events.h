#pragma once
#include "EventBus/Event.hpp"
#include "EventBus/Object.hpp"
#include "../Network/NetworkManager.h"

class ConnectionEvent : public Event
{
public:
	ConnectionEvent(ObjectPtr sender, NetworkType network) :
		Event(sender),
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
	DisconnectionEvent(ObjectPtr sender, NetworkType network) :
		Event(sender),
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
	MessageSentEvent(ObjectPtr sender, std::string message, bool source) :
		Event(sender),
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

class MessageRecievedEvent : public Event
{
public:
	MessageRecievedEvent(ObjectPtr sender) :
		Event(sender) {

	}
	~MessageRecievedEvent() {}
};

class SMSRecievedEvent : public Event
{
public:
	SMSRecievedEvent(ObjectPtr sender) :
		Event(sender) {
	}
	~SMSRecievedEvent() {}
};

