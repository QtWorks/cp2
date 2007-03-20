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
	std::string s = std::string(_settings.value(key.c_str(), 
		defaultValue.c_str()).toString().toAscii());

	_settings.setValue(key.c_str(), s.c_str());
	_settings.sync();

	return s;
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
	double d = _settings.value(key.c_str(), defaultValue).toDouble();
	_settings.setValue(key.c_str(), d);
	_settings.sync();
	return d;
}

//////////////////////////////////////////////////////////
void
CP2Config::setInt(std::string key, int value) 
{
	_settings.setValue(key.c_str(), value);
	_settings.sync();
}

//////////////////////////////////////////////////////////
int
CP2Config::getInt(std::string key, int defaultValue) 
{
	int i = _settings.value(key.c_str(), defaultValue).toDouble();
	_settings.setValue(key.c_str(), i);
	_settings.sync();
	return i;
}

//////////////////////////////////////////////////////////
void
CP2Config::setBool(std::string key, bool value) 
{
	_settings.setValue(key.c_str(), value);
	_settings.sync();
}

//////////////////////////////////////////////////////////
bool
CP2Config::getBool(std::string key, bool defaultValue) 
{
	bool b = _settings.value(key.c_str(), defaultValue).toBool();
	_settings.setValue(key.c_str(), b);
	_settings.sync();
	return b;
}

//////////////////////////////////////////////////////////
void
CP2Config::sync() 
{
	_settings.sync();
}
