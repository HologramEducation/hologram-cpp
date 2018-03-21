#pragma once
#include "../../../../../Utils/Utils.h"

class IP {
public: 
	virtual bool connect() = 0;
	virtual void disconnect() = 0;
};