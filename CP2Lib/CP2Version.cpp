#include "CP2Version.h"
#include "svnInfo.h"

std::string
CP2Version::revision() {

	return std::string(SVNREVISION);

}
