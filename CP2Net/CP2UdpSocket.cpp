#include "CP2UdpSocket.h"
#include <string>
#include <QList>
#include <QNetworkInterface>
#include <QNetworkAddressEntry>
#include <winsock2.h>


///////////////////////////////////////////////////////////
CP2UdpSocket::CP2UdpSocket(
						   std::string network, 
						   int port,
						   bool broadcast, 
						   int sndBufferSize, 
						   int rcvBufferSize):
_network(network),
_port(port),
_broadcast(broadcast),
_sndBufferSize(sndBufferSize),
_rcvBufferSize(rcvBufferSize),
_ok(false)
{
	QList<QNetworkInterface> allIfaces = QNetworkInterface::allInterfaces();

	QNetworkAddressEntry addrEntry;
	int i;
	for (i = 0; i < allIfaces.size(); i++) {
		QNetworkInterface iface = allIfaces[i];
		QList<QNetworkAddressEntry> addrs = iface.addressEntries();
		for (int j = 0; j < addrs.size(); j++) {
			std::string thisIp = addrs[j].ip().toString().toAscii();
			if (thisIp.find(_network)!= std::string::npos) {
				addrEntry = addrs[j];
				break;
			}
		}
	}

	if (i == allIfaces.size())
		return;

	if (!broadcast) {
		_hostAddress = addrEntry.ip();
	} else {
		_hostAddress = addrEntry.broadcast();
	}

	// bind socket to port/network
	int optval = 1;
	if (!bind(_hostAddress, _port)) {
		qWarning("Unable to bind to %s:%d", 
			_hostAddress.toString().toAscii().constData(), 
			_port);
	}
	int result = setsockopt(socketDescriptor(), 
		SOL_SOCKET, 
		SO_REUSEADDR, 
		(const char*)&optval, 
		sizeof(optval)); 

	if (_sndBufferSize) {
		// set the system send buffer size
		int sockbufsize = _sndBufferSize;
		result = setsockopt (socketDescriptor(),
			SOL_SOCKET,
			SO_SNDBUF,
			(char *) &sockbufsize,
			sizeof sockbufsize);
		if (result) {
			qWarning("Set send buffer size for socket failed");
		}
	}

	if (_rcvBufferSize) {
		// set the system receive buffer size
		int sockbufsize = _rcvBufferSize;
		result = setsockopt (socketDescriptor(),
			SOL_SOCKET,
			SO_RCVBUF,
			(char *) &sockbufsize,
			sizeof sockbufsize);
		if (result) {
			qWarning("Set receive buffer size for socket failed");
		}
	}

	_ok = true;
}

///////////////////////////////////////////////////////////
CP2UdpSocket::~CP2UdpSocket()
{
}

///////////////////////////////////////////////////////////
bool
CP2UdpSocket::ok()
{
	return _ok;
}
///////////////////////////////////////////////////////////
std::string
CP2UdpSocket::toString()
{
	return _hostAddress.toString().toAscii().constData();
}

///////////////////////////////////////////////////////////
QHostAddress
CP2UdpSocket::hostAddress()
{
	return _hostAddress;
}


