/*
 * =====================================================================================
 *  FILENAME	:  SocketWF.h
 *  DESCRIPTION	:  This file defines Wrapper Facades classes for Socket APIs.
 *  			   It is used to hide some UNIX/POSIX and Win32 Socket APIs.
 *  			   Classes:
 *  	 - INET_Addr: encapsulates IPv4 address
 *       - SOCK_Acceptor: connection factory for TCP. It encapsulates listening
 *  	   socket and waiting for request connection from clients. A server just needs
 *  	   one SOCK_Acceptor object for listening to clients on a particular address.
 *  	 - SOCK_Stream: encapsulates connected TCP descriptor and some function
 *  	   for sending and receiving data through that descriptor. Each connected
 *  	   socket corresponds to a SOCK_Stream object.
 *  	 -SOCK_Datagram: encapsulates UDP socket. Server app just needs to instantiate
 *  	  one SOCK_Datagram object for listening all clients on one address.
 *
 *  VERSION		:  1.0
 *  CREATED		:  12/03/2010 02:25:17 PM
 *  REVISION	:  none
 *  COMPILER	:  g++
 *  AUTHOR		:  Ngoc Son
 *  COPYRIGHT	:  Copyright (c) 2010, Ngoc Son
 *
 * =====================================================================================
 */
#ifndef SOCKET_WF_H_
#define SOCKET_WF_H_

#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include "reactor_type.h"


/*
 * =====================================================================================
 *        Class:  INET_Addr
 *  Description:  INET_Addr class encapsulates Internet address structure
 * =====================================================================================
 */
class INET_Addr {
	private:
		struct sockaddr_in mAddr; //IPv4 structure

	public:
		INET_Addr(uint16_t port){
			memset(&mAddr, 0x00, sizeof(mAddr));
			mAddr.sin_family = AF_INET;
			mAddr.sin_port = htons(port);
			mAddr.sin_addr.s_addr = INADDR_ANY; //listen on all interfaces
		}

		INET_Addr(uint16_t port, uint32_t addr) {
			memset(&mAddr, 0x00, sizeof(mAddr));
			mAddr.sin_family = AF_INET; //choose Internet address
			mAddr.sin_port = htons(port);
			mAddr.sin_addr.s_addr = htonl(addr); //listen on a particular interfaces
		}

		uint16_t mGetPort() {
			return mAddr.sin_port;
		}

		uint32_t mGetIpAddr() {
			return mAddr.sin_addr.s_addr;
		}

		struct sockaddr* mGetAddr() const{
			//return reinterpret_cast<struct sockaddr*> (&mAddr);
			return (struct sockaddr*)&mAddr;
		}

		socklen_t mGetSize() const {
			return sizeof(mAddr);
		}
};


/*
 * =====================================================================================
 *        Class:  SOCK_Stream
 *  Description:  SOCK_Stream class encapsulates the I/O operations that an 
 *  			  application can invoke on a connected socket handle. 
 *  			  Each SOCK_Stream object is correspond to a TCP connection.
 * =====================================================================================
 */
class SOCK_Stream {
	private:
		//It is connected socket
		SOCKET mHandle;
		struct sockaddr_in mPeerAddr;
		socklen_t mPeerAddrLen;

	public:
		SOCK_Stream() {
			//set default mHandle to -1
			mHandle = INVALID_HANDLE_VALUE;
		}

		SOCK_Stream(SOCKET h) {
			mHandle = h;
		}

		//Automatically close the handle on destructor
		~SOCK_Stream() {
			close(mHandle);
		}

		//Set the SOCKET handle
		void mSetHandle(SOCKET connsock) {
			mHandle = connsock;
		}

		//Set connected socket descriptor and peer address structure
		void mSetPeer(SOCKET connsock, struct sockaddr_in* cliaddr, socklen_t clilen){
			mHandle = connsock;
			memcpy(&mPeerAddr, cliaddr, sizeof(mPeerAddr));
			mPeerAddrLen = clilen;
		}

		//Get the SOCKET handle
		SOCKET mGetHandle() const {
			return mHandle;
		}

		//Normal I/O operations
		ssize_t mRecv(void* buf, size_t len, int flags){
		}

		ssize_t mSend(const char* buf, size_t len, int flags){
		}

		//I/O operations for short receives and sends
		ssize_t mRecv_n(char* buf, size_t len, int flags);
		ssize_t mSend_n(const char* buf, size_t len, int flags);

		//Other methods
};


/*
 * =====================================================================================
 *        Class:  SOCK_Acceptor
 *  Description:  SOCK_Acceptor is connection factory which accept connect from client
 *  			  and initializes SOCK_Stream object for further I/O operations.
 *  			  This is implementation of TCP listening socket.
 * =====================================================================================
 */
class SOCK_Acceptor {
	private:
		//Socket handle factory. It is listening socket
		SOCKET mHandle;

	public:
		//Constructor initializes listenning socket
		SOCK_Acceptor(const INET_Addr& addr) {
			//create server socket, use streaming socket (TCP)
			mHandle = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
			//bind between server socket and Internet address
			bind(mHandle, addr.mGetAddr(), addr.mGetSize());
			//change server socket to listenning mode
			listen(mHandle, BACKLOG);
		}

		//A second method to initialize a passive-mode acceptor
		//socket, analogously to the constructor
		void mOpen(const INET_Addr &sock_addr) {			
		}

		//Accept a connection and initialize the SOCK_Stream
		void mAccept(SOCK_Stream* stream) {
			struct sockaddr_in cliaddr;
			socklen_t clilen = sizeof(cliaddr);

			SOCKET conn = accept(mHandle, (struct sockaddr*)&cliaddr, &clilen);
			//stream->mSetHandle(conn);
			stream->mSetPeer(conn, &cliaddr, clilen);
//			pantheios::log_INFORMATIONAL("Set SOCK_Stream with Connection FD: ", pantheios::integer(conn));
		}

		SOCKET mGetHandle() const {
			return mHandle;
	    }
};

/*
 * =====================================================================================
 *        Class:  SOCK_Datagram
 *  Description:  This class used as wrapper for datagram transport protocol such as UDP
 * =====================================================================================
 */
class SOCK_Datagram {
	private:
		SOCKET mHandle;

	public:
		SOCK_Datagram(const INET_Addr& addr){
			mHandle = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
			bind(mHandle, addr.mGetAddr(), addr.mGetSize());
		}
		
		SOCKET mGetHandle() const{
			return mHandle;
		}

		ssize_t mRecvfrom(void* buff, size_t nbytes, int flags, struct sockaddr* from, socklen_t* len){
			return recvfrom(mHandle, buff, nbytes, flags, from, len);
		}

		ssize_t mSendto(const void* buff, size_t nbytes, int flags, const struct sockaddr* to, socklen_t len){
			return sendto(mHandle, buff, nbytes, flags, to, len);
		}
};

#endif // SOCKET_WF_H_
