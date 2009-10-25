// $Id$

#include "MSXCommandController.hh"
#include "GlobalCommandController.hh"
#include "Reactor.hh"
#include "MSXEventDistributor.hh"
#include "MSXMotherBoard.hh"
#include "SettingsConfig.hh"
#include "SettingsManager.hh"
#include "InfoCommand.hh"
#include "Interpreter.hh"
#include "Setting.hh"
#include "Event.hh"
#include <iostream>

using std::string;
using std::vector;

namespace openmsx {

MSXCommandController::MSXCommandController(
		GlobalCommandController& globalCommandController_,
		Reactor& reactor_,
		MSXMotherBoard& motherboard_,
		MSXEventDistributor& msxEventDistributor_,
		const std::string& machineID_)
	: globalCommandController(globalCommandController_)
	, reactor(reactor_)
	, motherboard(motherboard_)
	, msxEventDistributor(msxEventDistributor_)
	, machineID(machineID_)
{
	getInterpreter().createNamespace(machineID);

	machineInfoCommand.reset(new InfoCommand(*this, "machine_info"));

	msxEventDistributor.registerEventListener(*this);
}

MSXCommandController::~MSXCommandController()
{
	msxEventDistributor.unregisterEventListener(*this);

	machineInfoCommand.reset();

	#ifndef NDEBUG
	for (CommandMap::const_iterator it = commandMap.begin();
	     it != commandMap.end(); ++it) {
		std::cout << "Command not unregistered: " << it->first << std::endl;
	}
	for (SettingMap::const_iterator it = settingMap.begin();
	     it != settingMap.end(); ++it) {
		std::cout << "Setting not unregistered: " << it->first << std::endl;
	}
	assert(commandMap.empty());
	assert(settingMap.empty());
	#endif

	getInterpreter().deleteNamespace(machineID);
}

GlobalCommandController& MSXCommandController::getGlobalCommandController()
{
	return globalCommandController;
}

InfoCommand& MSXCommandController::getMachineInfoCommand()
{
	return *machineInfoCommand;
}

void MSXCommandController::registerCommand(Command& command, const string& str)
{
	assert(!hasCommand(str));
	commandMap[str] = &command;

	string fullname = "::" + machineID + "::" + str;
	globalCommandController.registerCommand(command, fullname);
	globalCommandController.registerProxyCommand(str);
}

void MSXCommandController::unregisterCommand(Command& command, const string& str)
{
	assert(hasCommand(str));
	commandMap.erase(str);

	globalCommandController.unregisterProxyCommand(str);
	string fullname = "::" + machineID + "::" + str;
	globalCommandController.unregisterCommand(command, fullname);
}

void MSXCommandController::registerCompleter(CommandCompleter& completer,
                                             const string& str)
{
	string fullname = "::" + machineID + "::" + str;
	globalCommandController.registerCompleter(completer, fullname);
}

void MSXCommandController::unregisterCompleter(CommandCompleter& completer,
                                               const string& str)
{
	string fullname = "::" + machineID + "::" + str;
	globalCommandController.unregisterCompleter(completer, fullname);
}

void MSXCommandController::registerSetting(Setting& setting)
{
	string name = setting.getName();
	assert(!findSetting(name));
	settingMap[name] = &setting;

	globalCommandController.registerProxySetting(setting);
	string fullname = "::" + machineID + "::" + name;
	getSettingsConfig().getSettingsManager().registerSetting(setting, fullname);
	getInterpreter().registerSetting(setting, fullname);
}

void MSXCommandController::unregisterSetting(Setting& setting)
{
	string name = setting.getName();
	assert(findSetting(name));
	settingMap.erase(name);

	globalCommandController.unregisterProxySetting(setting);
	string fullname = "::" + machineID + "::" + name;
	getInterpreter().unregisterSetting(setting, fullname);
	getSettingsConfig().getSettingsManager().unregisterSetting(setting, fullname);
}

void MSXCommandController::changeSetting(Setting& setting, const string& value)
{
	string fullname = "::" + machineID + "::" + setting.getName();
	globalCommandController.changeSetting(fullname, value);
}

string MSXCommandController::makeUniqueSettingName(const string& name)
{
	//return getSettingsConfig().getSettingsManager().makeUnique(name);
	return globalCommandController.makeUniqueSettingName(name);
}

Command* MSXCommandController::findCommand(const std::string& name) const
{
	CommandMap::const_iterator it = commandMap.find(name);
	return (it != commandMap.end()) ? it->second : NULL;
}

Setting* MSXCommandController::findSetting(const std::string& name)
{
	SettingMap::const_iterator it = settingMap.find(name);
	return (it != settingMap.end()) ? it->second : NULL;
}

bool MSXCommandController::hasCommand(const string& command) const
{
	return findCommand(command) != NULL;
}

string MSXCommandController::executeCommand(const string& command,
                                            CliConnection* connection)
{
	return globalCommandController.executeCommand(command, connection);
}

void MSXCommandController::tabCompletion(std::string& command)
{
	globalCommandController.tabCompletion(command);
}

bool MSXCommandController::isComplete(const std::string& command)
{
	return globalCommandController.isComplete(command);
}

void MSXCommandController::splitList(const string& list,
	                             vector<string>& result)
{
	globalCommandController.splitList(list, result);
}

CliComm& MSXCommandController::getCliComm()
{
	return motherboard.getMSXCliComm();
}

Interpreter& MSXCommandController::getInterpreter()
{
	return globalCommandController.getInterpreter();
}

SettingsConfig& MSXCommandController::getSettingsConfig()
{
	return globalCommandController.getSettingsConfig();
}

void MSXCommandController::signalEvent(
	shared_ptr<const Event> event, EmuTime::param /*time*/)
{
	if (event->getType() != OPENMSX_MACHINE_ACTIVATED) return;

	// simple way to synchronize proxy settings
	for (SettingMap::const_iterator it = settingMap.begin();
	     it != settingMap.end(); ++it) {
		changeSetting(*it->second, it->second->getValueString());
	}
}

bool MSXCommandController::isActive() const
{
	return reactor.getMotherBoard() == &motherboard;
}

} // namespace openmsx
