#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
extern int errno;

#include "../network.h" 

/// This machine's IP address
unsigned char NETWORKINIT::pMyIP[4];

/// Stats
unsigned int NETWORKINIT::pSent;
unsigned int NETWORKINIT::pRecv;

/// Creates a generic network stream
NETSTREAM::NETSTREAM()
{
	closed = true;
	handle = 0;
}

/// Destroys the stream
NETSTREAM::~NETSTREAM()
{
	if (handle != 0)
	{
		shutdown(handle, 2);
		::close(handle);
	}
}

/// Closes the stream
void NETSTREAM::close()
{
	closed = true;
}

/// Gets the IP address
void NETSTREAM::getIPAddress(unsigned char ip[4])
{
	ip[0] = ip[1] = ip[2] = ip[3] = 0;
	if (!isValid())
		return;

    long nl = sizeof(struct sockaddr_in);
    struct sockaddr_in adr;

    if (getpeername(handle, (struct sockaddr *)&adr, (socklen_t *)&nl))
        return;

    unsigned long oadr = ntohl(adr.sin_addr.s_addr);
	ip[0] = (unsigned char)((oadr >> 24) & 0xFF);
	ip[1] = (unsigned char)((oadr >> 16) & 0xFF);
	ip[2] = (unsigned char)((oadr >> 8) & 0xFF);
	ip[3] = (unsigned char)((oadr) & 0xFF);
}

/// Creates a buffered network stream
BUFFNETSTREAM::BUFFNETSTREAM()
{
	sendBuf = new char[4096 + sizeof(int) * 2];
	recBuf = new char[16384];
	userData = NULL;
	recLength = 0;
}

/// Destroys the buffered net stream
BUFFNETSTREAM::~BUFFNETSTREAM()
{
	delete [] sendBuf;
	delete [] recBuf;
}

/// Starts to send a message to the other end of this stream.
char *BUFFNETSTREAM::beginSendMsg(const int type)
{
	*(((int *)sendBuf) + 0) = type;
	*(((int *)sendBuf) + 1) = 0;
	return(sendBuf + sizeof(int) * 2);
}

/// Finishes to send the message
void BUFFNETSTREAM::endSendMsg(const int length)
{
	*(((int *)sendBuf) + 1) = length;
	if (handle == 0 || closed)
		return;

	NETWORKINIT::pSent += (length + sizeof(int) * 2);
	int res = send(handle, sendBuf, length + sizeof(int) * 2, 0);
	
	if (res == SOCKERR)
	{
		int error = errno;

		while (error == EWOULDBLOCK)
		{
			int res = send(handle, sendBuf, length + sizeof(int) * 2, 0);
			if (res != SOCKERR)
				return;
			error = errno;
		}
/*
		switch (error)
		{
			case (WSANOTINITIALISED):
				//Err("Network error: network not initialised.");
				break;
			case (WSAENETDOWN):
				//Err("Network error: network is not available.");
				break;
			case (WSAEAFNOSUPPORT):
				//Err("Network error: family address not supported.");
				break;
			case (WSAEINPROGRESS):
				//Err("Network error: blocking call.");
				break;
			case (WSAEMFILE):
				//Err("Network error: No more socket descriptors available.");
				break;
			case (WSAENOBUFS):
				//Err("Network error: No buffer space available.");
				break;
			case (WSAEPROTONOSUPPORT):
				//Err("Network error: Protocol not supported.");
				break;
			case (WSAEPROTOTYPE):
				//Err("Network error: Wrong protocol for the socket.");
				break;
			case (WSAESOCKTNOSUPPORT):
				//Err("Network error: socket type not supported by this family.");
				break;
			case (WSAEFAULT):
				//Err("Network error: memory exception.");
				break;
			case (WSAENOTCONN):
				//Err("Network error: socket not connected.");
				break;
			case (WSAEINTR):
				//Err("Network error: blocking call canceled.");
				break;
			case (WSAENETRESET):
				//Err("Network error: connection lost due to host resetting.");
				break;
			case (WSAENOTSOCK):
				//Err("Network error: not a valid socket.");
				break;
			case (WSAEOPNOTSUPP):
				//Err("Network error: MSG_OOB impossible on a such stream.");
				break;
			case (WSAESHUTDOWN):
				//Err("Network error: socket shutdowned.");
				break;
			case (WSAEWOULDBLOCK):
				//Err("Network error: operation should block.");
				break;
			case (WSAEMSGSIZE):
				//Err("Network error: message too large.");
				break;
			case (WSAEINVAL):
				//Err("Network error: socket not bound.");
				break;
			case (WSAECONNABORTED):
				//Err("Network error: timed-out socket, should close.");
				break;
			case (WSAETIMEDOUT):
				//Err("Network error: peer system failed to respond.");
				break;
			case (WSAECONNRESET):
				//Err("Network error: virtual circuit closed.");
				break;
			case (WSAEACCES):
				//Err("Network error: broadcast without proper flags.");
				break;
			case (WSAEHOSTUNREACH):
				//Err("Network error: unreacheable host.");
				break;
		}
*/
		closed = true;
		return;
	}
}

///	Checks if a message is ready to be received
bool BUFFNETSTREAM::hasRecvMsg()
{
	if (recLength < sizeof(int) * 2)
		return false;

	int l = *(((int *)recBuf) + 1);
	return(recLength >= l + sizeof(int) * 2);
}

/// Starts to receive a message
char *BUFFNETSTREAM::beginRecvMsg(int& type, int& len)
{
	if (recLength < sizeof(int) * 2)
		return NULL;

	len = *(((int *)recBuf) + 1);
	type = *(((int *)recBuf) + 0);
	if (recLength >= len + sizeof(int) * 2)
		return(recBuf + sizeof(int) * 2);
	else
		return NULL;
}

/// Finishes to receive the message
void BUFFNETSTREAM::endRecvMsg()
{
	int tl = *(((int *)recBuf) + 1);
	tl += sizeof(int) * 2;
	recLength -= tl;
	memcpy(recBuf, recBuf + tl, recLength);
}

/// Sets the custom user data
void BUFFNETSTREAM::setUserData(void *data)
{
	userData = data;
}

/// Updates the stream
void BUFFNETSTREAM::update()
{
	int l = 16384;
	while (l > 0 && recLength < 16384)
	{
		l = recv(handle, recBuf + recLength, 16384 - recLength, 0);
		if (l == SOCKERR)
		{
			int error = error;
			if (error == EWOULDBLOCK)
				/// we haven't received anything => quit
				break;

			closed = true;
			break;
		}
		else
		{
			recLength += l;
			NETWORKINIT::pRecv += l;
		}
	}
}

/// Creates a client stream
CLIENTSTREAM::CLIENTSTREAM(char *hostName, const int port)
{
    struct hostent *shost;
    struct sockaddr_in adrserver;

	if (hostName[0] >= '0' && hostName[0] <= '9')
	{
		int count = 0;
		for (unsigned int i = 0; i < strlen(hostName); i++)
		{
			if (hostName[i] == '.')
				count++;
		}

		if (count == 4)
			/// IP name
			shost = gethostbyaddr(hostName, strlen(hostName), AF_INET);
		else
		    shost = gethostbyname(hostName);
	}
	else
	    shost = gethostbyname(hostName);

    if (!shost)
    {
        /// Host name not found. Can't create socket.
        handle = 0;
        return;
    }
//iqa
    handle = socket(AF_INET, SOCK_STREAM, 0);
    if (handle == -1)
    {
        handle = 0;
        return;
    }

    memset(&addr, 0, sizeof(struct sockaddr));
    struct sockaddr_in *sin = (struct sockaddr_in *)&addr;

    sin->sin_family = AF_INET;
    sin->sin_addr.s_addr = INADDR_ANY;
    sin->sin_port = htons(0);
    addrLength = sizeof(struct sockaddr_in);

    if (bind(handle, &addr, sizeof(sockaddr_in)) == SOCKERR)
    {
        handle = 0;
		return;
    }

    adrserver.sin_family = AF_INET;
    adrserver.sin_port = htons(port);
    memcpy((void *)&(adrserver.sin_addr.s_addr),(void *)shost->h_addr,
        (size_t)shost->h_length);

    if (connect(handle, (struct sockaddr *)(&adrserver), sizeof(struct sockaddr_in)))
	{
        handle = 0;
		return;
	}

	/// unblocks the socket:
	int flags = fcntl(handle, F_GETFL, 0);
	flags |= O_NONBLOCK;
	if (fcntl(handle, F_SETFL, flags) < 0)
	{
		//VRLog::print("socket: can't unblock socket");
		handle = 0;
		return;
	}

	/// validate the socket
	closed = false;
}

/// Creates a client stream, but on the server
SERVERINSTREAM::SERVERINSTREAM(SERVERSTREAM *server)
{
    struct sockaddr adr;
    int nl = 0;

    memset(&adr, 0, sizeof(struct sockaddr));

    if (server->handle == 0)
		return;

    SOCKET s = accept(server->handle, NULL, NULL);
	if (s == -1)
		return;

	handle = s;
	addr = adr;
	addrLength = nl;

	/// unblocks the socket:

    int flags = fcntl(handle, F_GETFL, 0);
	flags |= O_NONBLOCK;
	if (fcntl(handle, F_SETFL, flags) < 0)
	{
//		VRLog::print("socket: can't unblock socket");
		handle = 0;
		return;
	}

	/// validate the socket
	closed = false;
}

/// Creates a server stream
SERVERSTREAM::SERVERSTREAM(const int port)
{
    nbInStreams = 0;
    memset(inStreams, 0, sizeof(SERVERINSTREAM *) * 64);
    memset(streams, 0, sizeof(SERVERINSTREAM *) * 4096);

    handle = socket(AF_INET, SOCK_STREAM, 0);
    if (handle == SOCKERR)
    {
        handle = 0;
        return;
    }

    memset(&addr, 0, sizeof(struct sockaddr));
    struct sockaddr_in *sin = (struct sockaddr_in *)&addr;

    sin->sin_family = AF_INET;
    sin->sin_addr.s_addr = INADDR_ANY;
    sin->sin_port = htons(port);
    addrLength = sizeof(struct sockaddr_in);

    if (bind(handle, &addr, sizeof(sockaddr_in)) == -1)
    {
		handle = 0;
		return;
    }

    if (listen(handle, SOMAXCONN) == -1)
    {
		handle = 0;
		return;
    }

	/// unblocks the socket:
  	int flags = fcntl(handle, F_GETFL, 0);
	flags |= O_NONBLOCK;
	if (fcntl(handle, F_SETFL, flags) < 0)
	{
		//VRLog::print("socket: can't unblock socket");
		handle = 0;
	}

	/// validate the socket
	closed = false;
}

/// Updates the server stream
void SERVERSTREAM::update()
{
	fd_set inputSet;
	fd_set outputSet;
	fd_set excSet;
	memset(&inputSet, 0, sizeof(fd_set));
	memset(&outputSet, 0, sizeof(fd_set));
	memset(&excSet, 0, sizeof(fd_set));

    FD_SET(handle, &inputSet);
    FD_SET(handle, &excSet);

    struct timeval stv;
    stv.tv_sec = 0;
    stv.tv_usec = 0;
	select(0, &inputSet, &outputSet, &excSet, &stv);

	if (nbInStreams < 64 && FD_ISSET(handle, &inputSet))
	{
		/// new connection incoming!
		SERVERINSTREAM *inStream = new SERVERINSTREAM(this);
		inStreams[nbInStreams++] = inStream;
	}

	/// also update all the client connections
	for (int i = 0; i < 4096; i++)
	{
		if (streams[i] != NULL)
		{
			streams[i]->update();
			if (streams[i]->closed && streams[i]->handle != 0)
			{
				streams[i] = NULL;
			}
		}
	}
}

/// Gets a new connected stream, or NULL if none is available
/// Warning: LIFO type (assumes clients are "safe")
SERVERINSTREAM *SERVERSTREAM::detectNewConnections()
{
    int i;
    
	if (nbInStreams <= 0)
		return NULL;

	SERVERINSTREAM *inStream = inStreams[nbInStreams - 1];
	inStreams[nbInStreams - 1] = NULL;
	nbInStreams--;

	/// adds this stream to the full list of streams
	for ( i = 0; i < 4096; i++)
	{
		if (streams[i] == NULL)
		{
			streams[i] = inStream;
			break;
		}
	}
	if (i >= 4096)
	{
		/// not enough room on server
		delete inStream;
		return NULL;
	}
	return inStream;
}

static NETWORKINIT NETWORKINIT_INSTANCE;

/// Automatic initialization of the networking sub system
NETWORKINIT::NETWORKINIT()
{
/*
    short                wVersionRequested;
    WSADATA             wsaData;

    wVersionRequested = MAKEWORD(1,1);
    if (WSAStartup(wVersionRequested, &wsaData))
	{
	}
*/

// IRIX!
//    sigignore(SIGPIPE);


	/// Get this machine's IP address
    char myname[256];
    gethostname(myname, 255);
    struct hostent *info = gethostbyname(myname);
    unsigned char *adrInfo = (unsigned char *)(info->h_addr);
	memcpy(pMyIP, adrInfo, 4);
}

/// Creates a multicast object
MULTICAST::MULTICAST()
{
    sendHandle = socket(AF_INET, SOCK_DGRAM, 0);
    if (sendHandle == -1)
    {
        sendHandle = 0;
        return;
    }

    recvHandle = socket(AF_INET, SOCK_DGRAM, 0);
    if (recvHandle == -1)
    {
        recvHandle = 0;
        return;
    }

    /// Server mode
    struct sockaddr_in addrLocal;
    addrLocal.sin_family = AF_INET;
    addrLocal.sin_addr.s_addr = htonl(INADDR_ANY);
    addrLocal.sin_port = 0;

    if (bind(sendHandle, (struct sockaddr *)&addrLocal,
        sizeof(struct sockaddr)) == SOCKERR)
    {
        sendHandle = 0;
        return;
    }

    struct ip_mreq mreq;
    mreq.imr_multiaddr.s_addr = inet_addr("234.5.6.7");
    mreq.imr_interface.s_addr = INADDR_ANY;
    if (setsockopt(sendHandle, IPPROTO_IP,IP_ADD_MEMBERSHIP, (char *)&mreq,
        sizeof(mreq)) == SOCKERR)
    {
        sendHandle = 0;
        return;
    }

    unsigned long ttl = 32;
    if (setsockopt(sendHandle, IPPROTO_IP, IP_MULTICAST_TTL, (char *)&ttl,
        sizeof(ttl)) == SOCKERR)
    {
        sendHandle = 0;
        return;
    }

    /// we don't care if this one succeeds..
    bool flag = false;
    setsockopt(sendHandle, IPPROTO_IP, IP_MULTICAST_LOOP, (char *)&flag,
        sizeof(flag));

    int flags = fcntl(sendHandle, F_GETFL, 0);
    flags |= O_NONBLOCK;
    if (fcntl(sendHandle, F_SETFL, flags) < 0)
	{
 	sendHandle = 0;
	}

	/// Client mode

    flag = true;
    if (setsockopt(recvHandle, SOL_SOCKET, SO_REUSEADDR, (char *)&flag,
        sizeof(flag)) == SOCKERR)
    {
        recvHandle = 0;
        return;
    }

    addrLocal.sin_family = AF_INET;
    addrLocal.sin_addr.s_addr = htonl(INADDR_ANY);
    addrLocal.sin_port = htons(6669);

    if (bind(recvHandle, (struct sockaddr *)&addrLocal,
        sizeof(struct sockaddr)) == SOCKERR)
    {
        recvHandle = 0;
        return;
    }

    mreq.imr_multiaddr.s_addr = inet_addr("234.5.6.7");
    mreq.imr_interface.s_addr = INADDR_ANY;
    if (setsockopt(recvHandle, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&mreq,
        sizeof(mreq)) == SOCKERR)
    {
        recvHandle = 0;
        return;
    }

     flags = fcntl(recvHandle, F_GETFL, 0);
	flags |= O_NONBLOCK;
	if (fcntl(recvHandle, F_SETFL, flags) < 0)
	{
		recvHandle = 0;
	}
}

/// Destroys it
MULTICAST::~MULTICAST()
{
    struct ip_mreq mreq;
    mreq.imr_multiaddr.s_addr = inet_addr("234.5.6.7");
    mreq.imr_interface.s_addr = INADDR_ANY;

    if (setsockopt(sendHandle, IPPROTO_IP, IP_DROP_MEMBERSHIP, (char *)&mreq,
        sizeof(mreq)) == 0)
	    close(sendHandle);

    if (setsockopt(recvHandle, IPPROTO_IP, IP_DROP_MEMBERSHIP, (char *)&mreq,
        sizeof(mreq)) == 0)
	    close(recvHandle);

    sendHandle = 0;
    recvHandle = 0;
}

/// Sends a multicast packet to the LAN
void MULTICAST::send(const unsigned char ip[4], const int toPort)
{
	if (sendHandle == 0)
		return;

	unsigned char buffer[256];
	buffer[0] = ip[0];
	buffer[1] = ip[1];
	buffer[2] = ip[2];
	buffer[3] = ip[3];
	int *ibuffer = (int *)(buffer + 4);
	ibuffer[0] = toPort;

    struct sockaddr_in addrDst;
    addrDst.sin_family = AF_INET;
    addrDst.sin_addr.s_addr = inet_addr("234.5.6.7");
    addrDst.sin_port = htons(6669);

    sendto(sendHandle, (char *)buffer, 4 + sizeof(int), 0, (struct sockaddr *)
        &addrDst, sizeof(addrDst));
}

/// Receives a multicast packet from the LAN
bool MULTICAST::recv(unsigned char *ip, int& port)
{
    if (recvHandle == 0)
        return(false);

	unsigned char buffer[256];
	struct sockaddr_in addrSrc;
    int addrSize = sizeof(struct sockaddr_in);
    int ret = recvfrom(recvHandle, (char *)buffer, 
    4 + sizeof(int), 0,	0, 0);
	if (ret != 4 + sizeof(int))
		return(false);

	ip[0] = buffer[0];
	ip[1] = buffer[1];
	ip[2] = buffer[2];
	ip[3] = buffer[3];
	int *ibuffer = (int *)(buffer + 4);
	port = ibuffer[0];

	return true;
}
 
