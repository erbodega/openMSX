// $Id$

#include "MSXTurboRPause.hh"


MSXTurboRPause::MSXTurboRPause(Device *config, const EmuTime &time)
	: MSXDevice(config, time), MSXIODevice(config, time)
{
	reset(time);
}

MSXTurboRPause::~MSXTurboRPause()
{
}
 
void MSXTurboRPause::reset(const EmuTime &time)
{
	status = 0;
}

byte MSXTurboRPause::readIO(byte port, const EmuTime &time)
{
	return status;
}


// TODO implement "turborpause" command
