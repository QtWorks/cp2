#include "CP2UdpSocket.h"
#include <string>
#include <iostream>
#include <QList>
#include <QNetworkInterface>
#include <QNetworkAddressEntry>
#ifdef WIN32
#include <winsock2.h>
#else
#include <sys/types.h>
#include <sys/socket.h>
#endif

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
	bool found = false;
	for (i = 0; i < allIfaces.size(); i++) {
		QNetworkInterface iface = allIfaces[i];
		QList<QNetworkAddressEntry> addrs = iface.addressEntries();
		for (int j = 0; j < addrs.size(); j++) {
		  std::string thisIp = addrs[j].ip().toString().toStdString();
			if (thisIp.find(_network)!= std::string::npos) {
				addrEntry = addrs[j];
				found = true;
				break;
			}
		}
		if (found)
			break;
	}

	if (!found) {
		_errorMsg += "Unable to find interface for network ";
		_errorMsg += _network;
		_errorMsg += "\n";
		return;
	}

	if (!broadcast) {
		_hostAddress = addrEntry.ip();
	} else {
		/// @todo Find out why QNetworkAddressEntry.broadcast() doesn't work,
		/// or how it is supposed to be used.
		_hostAddress = QHostAddress(addrEntry.ip().toIPv4Address() | ~addrEntry.netmask().toIPv4Address());
	}

	// bind socket to port/network
	if (!broadcast) {
		if (!bind(_hostAddress, _port, QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint)) {
			_errorMsg += "Unable to bind datagram port ";
			_errorMsg += _hostAddress.toString().toStdString();
			_errorMsg += ":";
			_errorMsg += QString("%1").arg(_port).toStdString();
			_errorMsg += "\n";
			_errorMsg += this->errorString().toStdString();
			_errorMsg += "\n";
			return;
		}
	} else {
	  if (!bind(_port, QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint)) {
			_errorMsg += "Unable to bind datagram port ";
			_errorMsg += _hostAddress.toString().toStdString();
			_errorMsg += ":";
			_errorMsg += QString("%1").arg(_port).toStdString();
			_errorMsg += "\n";
			_errorMsg += this->errorString().toStdString();
			_errorMsg += "\n";
			return;
	  }
	}

	std::cout << "socket descriptor is " << socketDescriptor() << std::endl;

	int optval = 1;
	int result = setsockopt(socketDescriptor(), 
		SOL_SOCKET, 
		SO_REUSEADDR, 
		(const char*)&optval, 
		sizeof(optval)); 

	bool sockError = false;

	std::cout << "socket receive buffer size request " 
		     << _rcvBufferSize 
			<< ", send buffer size request " 
			   << _sndBufferSize <<  std::endl;

	if (_sndBufferSize) {
		// set the system send buffer size
		int sockbufsize = _sndBufferSize;
		result = setsockopt (socketDescriptor(),
			SOL_SOCKET,
			SO_SNDBUF,
			(char *) &sockbufsize,
			sizeof sockbufsize);
		if (result) {
			_errorMsg += "Set send buffer size for socket failed\n";
			perror("Setting socket send buffer size ");
			sockError = true;
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
			_errorMsg += "Set receive buffer size for socket failed\n";
			perror("Setting socket send buffer size ");
			sockError = true;
		}
	}

	if (sockError) {
	  return;
	}

	// read back the socket buffer sizes
	int sockbufsize;
	int sz;

	result = getsockopt (socketDescriptor(),
			     SOL_SOCKET,
			     SO_SNDBUF,
			     (char *) &sockbufsize,
			     &sz);

	std::cout << "socket send buffer size is " << sockbufsize << std::endl;


	result = getsockopt (socketDescriptor(),
			     SOL_SOCKET,
			     SO_RCVBUF,
			     (char *) &sockbufsize,
			     &sz);

	std::cout << "socket receive buffer size is " << sockbufsize << std::endl;


	_ok = true;

}

///////////////////////////////////////////////////////////
int
CP2UdpSocket::writeDatagram(const char* data, int size) 
{
	return QUdpSocket::writeDatagram(data, size, _hostAddress, _port);
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
CP2UdpSocket::errorMsg() {
	return _errorMsg;
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


