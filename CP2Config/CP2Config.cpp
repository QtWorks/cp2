#include "CP2Config.h"
#include <qlist>
#include <qvariant>

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
std::vector<CP2Config::TripleInt>
CP2Config::getArray(
		std::string key, 
		std::vector<CP2Config::TripleInt> defaultValues)
{
	std::vector<CP2Config::TripleInt> result;

	
	// Find out if the array exists, and its size
	int arraySize = _settings.beginReadArray(key.c_str());
	_settings.endArray();

	if (arraySize != defaultValues.size()) {
		// if the array doesn't exist, or does exist and
		// has a different size, remove the existing entries
		_settings.remove(key.c_str());
		// and add the default values
		_settings.beginWriteArray(key.c_str());
		for (unsigned int i = 0; i < defaultValues.size(); i++) {
			_settings.setArrayIndex(i);
			QString tripleString = QString("%1 %2 %3")
				.arg(defaultValues[i].values[0], 6)
				.arg(defaultValues[i].values[1], 6)
				.arg(defaultValues[i].values[2], 6);
			QList<QVariant> rgb;
			rgb.append(QVariant(defaultValues[i].values[0]));
			rgb.append(QVariant(defaultValues[i].values[1]));
			rgb.append(QVariant(defaultValues[i].values[2]));

			_settings.setValue("RGB", rgb);
			result.push_back(defaultValues[i]);
		}
		_settings.endArray();
	} else {
		_settings.beginReadArray(key.c_str());
		for (unsigned int i = 0; i < arraySize; i++) {
			_settings.setArrayIndex(i);
			QList<QVariant> rgb = _settings.value("RGB").toList();
			CP2Config::TripleInt data = {rgb[0].toInt(), rgb[1].toInt(), rgb[2].toInt()};
			result.push_back(data);
		}
	}

	return result;
}
//////////////////////////////////////////////////////////
void
CP2Config::sync() 
{
	_settings.sync();
}
