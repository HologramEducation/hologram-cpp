#pragma once
#include "..\Base\IP.h"

enum PPPType {
	PPPOS,
	PPPOE
};

class PPP : public IP
{
public:
	PPP(std::string name, std::string device) {
		type = PPPOS;
	}
	~PPP() {

	}
	virtual bool connect() {
		return false;
	}
	virtual void disconnect() {

	}
private:
	PPPType type;
};

