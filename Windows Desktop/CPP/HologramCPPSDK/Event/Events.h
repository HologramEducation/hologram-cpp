#pragma once
#include "EventBus/Event.h"
#include "../Network/NetworkManager.h"

class ConnectionEvent : public Event
{
public:
	ConnectionEvent( NetworkType network) :
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
	DisconnectionEvent( NetworkType network) :
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
	MessageSentEvent( std::wstring message, bool source) :
		message(message),
		source(source) {

	}
	~MessageSentEvent() {}

	std::wstring getMessage() {
		return message;
	}

	bool wasSentViaATSocket() {
		return source;
	}

private:
	std::wstring message;
	bool source;
};

class MessageRecievedEvent : public Event
{
public:
	MessageRecievedEvent(){

	}
	~MessageRecievedEvent() {}
};

class SMSRecievedEvent : public Event
{
public:
	SMSRecievedEvent() {

	}

	~SMSRecievedEvent() {}
};

