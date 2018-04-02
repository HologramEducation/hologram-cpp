#include "PPP.h"

PPP::PPP(Serial * serialport)
{
	this->serialport = serialport;
}

PPP::~PPP()
{
}
