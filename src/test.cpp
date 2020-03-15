/*
 * =====================================================================================
 *  FILENAME	:  main.c
 *  DESCRIPTION	:  Main function for testing. Open UDP and TCP on the same port.
 *  			   Use select() or poll() as demultiplexer.
 *  VERSION		:  1.0
 *  CREATED		:  12/03/2010 02:17:03 PM
 *  REVISION	:  none
 *  COMPILER	:  g++
 *  AUTHOR		:  Ngoc Son
 *  COPYRIGHT	:  Copyright (c) 2010, Ngoc Son
 *
 * =====================================================================================
 */
#include "Reactor.h"
#include "ReactorImpl.h"
#include "osip.h"
#include "osip_message.h"


//////////////////////////////////////
///		SAMPLE MAIN       ////////////
//////////////////////////////////////
const uint16_t PORT = 10000;
const PAN_CHAR_T	PANTHEIOS_FE_PROCESS_IDENTITY[] = "test.exe";

#define USE_EPOLL

void TCPreadCb(SOCKET socket, char* msg, ssize_t len);
void UDPreadCb(struct sockaddr_in, char* msg, ssize_t len);

int main(){
	int i;
	osip_t* osip;
	i = osip_init(&osip);
	if(i!=0)
		return -1;
	
	//Server address
	INET_Addr server_addr(PORT);

#ifdef USE_SELECT
	//we choose select() as demultiplexer
	DemuxType demux = SELECT_DEMUX;
#endif
#ifdef USE_POLL
	//we choose poll() as demultiplexer
	DemuxType demux = POLL_DEMUX;
#endif
#ifdef USE_EPOLL
	//we choose epoll() as demultiplexer
	DemuxType demux = EPOLL_DEMUX;
#endif
#ifdef USE_DEVPOLL
	//we choose /dev/poll as demultiplexer
	DemuxType demux = DEVPOLL_DEMUX;
#endif
#ifdef USE_KQUEUE
	//we choose kqueue as demultiplexer
	DemuxType demux = KQUEUE_DEMUX;
#endif

	//first call to sInstance() to instantiate sReactor and sReactorImpl.
	//"reactor" is pointer points to unique instance of Reactor class.
	//After calling this sInstance(), latter calls to sInstance() do not
	//create new instance of Reactor but just return this unique instance.
	Reactor* reactor = Reactor::sInstance(demux);

	//Create acceptor for TCP connection requests.
	//In this constructor, we call Reactor::sInstance() second time, so it just return us sReactor.
	ConnectionAcceptor connAcceptor(server_addr, reactor);
	
	//Register callback functions for incoming SIP messages and TCP events
	reactor->mRegisterTCPCbFuncs(TCPreadCb, NULL);

	//Create DgramHandler object for UDP datagrams which binding to server_addr address
	DgramHandler dgramHandler(server_addr, reactor);

	//Register callback functions for incoming UDP messages and UDP events
	reactor->mRegisterUDPCbFuncs(UDPreadCb, NULL);
	
	//Event loop that processes client connection and data reactively
	for(;;) {
		reactor->mHandleEvents();
	}

}

void TCPreadCb(SOCKET socket, char* msg, ssize_t len){
	//Process SIP message here
	
	osip_message_t* sip;
	int i;
	i = osip_message_init(&sip);
	if(i!=0){
//		pantheios::log_INFORMATIONAL("Cannot allocate !!!");
		return;
	}
	i = osip_message_parse(sip, msg, len);
	if(i!=0){
//		pantheios::log_INFORMATIONAL("Cannot parse SIP message !!!");
	}
	
	char* dest=NULL;
	size_t length=0;
	i = osip_message_to_str(sip, &dest, &length);
	if(i!=0){
//		pantheios::log_INFORMATIONAL("Cannot get printable message !!!");
		return;
	}
//	pantheios::log_INFORMATIONAL("TCP message: \n", dest);
	osip_free(dest);
	osip_message_free(sip);
	return;
}

void UDPreadCb(struct sockaddr_in peer, char* msg, ssize_t len){
//	pantheios::log_INFORMATIONAL("UDP message: \n", msg);
}
