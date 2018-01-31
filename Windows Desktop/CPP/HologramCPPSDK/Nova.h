#pragma once
#include "Modem.h"

class Nova : public Modem {
public:
	Nova();
	~Nova();

	bool sendMessage(std::string message);
private:
};