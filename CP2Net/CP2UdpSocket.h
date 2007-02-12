#ifndef CP2SOCKETINC_
#define CP2SOCKETINC_
#include <QUdpSocket>
#include <QHostAddress>
#include <string>
//#include <winsock.h>

class CP2UdpSocket: public QUdpSocket {
public:
	/// @param network Set this to the network IP of the interface
	/// that the socket will be bound to. The list of interfaces on the 
	/// system will be scanned for a match on this network. It is acceptable
	/// to only specify the network portion, such as 192.168.3, since
	/// on DHCP served systems, we typically won't know the full address.
	/// @param port The network port.
	/// @param broadcast Set true if the socket will be broadcasting
	/// @param sndBufferSize Set to non-zero to request the send buffer size
	/// to be set.
	/// @param rcvBroadcastSize Set to non-zero to request the receive buffer size 
	/// be set.
	CP2UdpSocket(std::string network, 
		int port,
		bool broadcast=false, 
		int sndBufferSize= 0, 
		int rcvBufferSize = 0);

	virtual ~CP2UdpSocket();

	/// @return The IP address for this socket.
	QHostAddress hostAddress();
	/// @return The IP number as a string
	std::string toString();
	/// @return true if the socket could be configured correctly, false otherwise
	bool ok();

protected:
	/// set true if the socket will be broadcasting
	bool _broadcast;
	/// Set to non-zero to request the send buffer size
	/// to be set.
	int _sndBufferSize;
	/// Set to non-zero to request the receive buffer size 
	/// be set.
	int _rcvBufferSize;
	/// The host address
	QHostAddress _hostAddress;
	/// The network interface
	std::string _network;
	/// The network port
	int _port;
	/// set true if socket is usable.
	bool _ok;
};

#endif