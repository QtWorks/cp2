#ifndef CP2CONFIGINC_
#define CP2CONFIGINC_

#include <QSettings.h>
#include <string>

class CP2Config {
public:
	CP2Config(const std::string organization, const std::string application);
	virtual ~CP2Config();

	void setString(std::string key, std::string t);
	std::string getString(std::string key, std::string defaultValue);

	void setDouble(std::string key, double t);
	double getDouble(std::string key, double defaultValue);

protected:
	/// The configuration permanent store
	QSettings _settings;

};


#endif
