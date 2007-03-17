#include "CP2Config.h"

//////////////////////////////////////////////////////////
CP2Config::CP2Config(const std::string organization, const std::string application):
_settings(QSettings::IniFormat, 
		  QSettings::UserScope, 
		  organization.c_str(), 
		  application.c_str())
{
}

//////////////////////////////////////////////////////////
CP2Config::~CP2Config() 
{
}

//////////////////////////////////////////////////////////
void
CP2Config::setString(std::string key, std::string t) 
{
	_settings.setValue(key.c_str(), t.c_str());
	_settings.sync();
}

//////////////////////////////////////////////////////////
std::string
CP2Config::getString(std::string key, std::string defaultValue) 
{
	return std::string(_settings.value(key.c_str(), 
		defaultValue.c_str()).toString().toAscii());
}

//////////////////////////////////////////////////////////
void
CP2Config::setDouble(std::string key, double value) 
{
	_settings.setValue(key.c_str(), value);
	_settings.sync();
}

//////////////////////////////////////////////////////////
double
CP2Config::getDouble(std::string key, double defaultValue) 
{
	return _settings.value(key.c_str(), defaultValue).toDouble();
}
