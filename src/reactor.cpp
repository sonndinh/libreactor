/*
 * =====================================================================================
 *  FILENAME	:  Reactor.c
 *  DESCRIPTION	:  
 *  VERSION		:  1.0
 *  CREATED		:  12/03/2010 02:22:17 PM
 *  REVISION	:  none
 *  COMPILER	:  g++
 *  AUTHOR		:  Ngoc Son
 *  COPYRIGHT	:  Copyright (c) 2010, Ngoc Son
 *
 * =====================================================================================
 */
#include "reactor.h"


/*
 *--------------------------------------------------------------------------------------
 *       Class:  ConnectionAcceptor
 *      Method:  ConnectionAcceptor :: ConnectionAcceptor()
 * Description:  Constructor which register handler to Reactor
 *--------------------------------------------------------------------------------------
 */
ConnectionAcceptor::ConnectionAcceptor(const INET_Addr &addr, Reactor* reactor){
	mReactor = reactor;
	mSockAcceptor = new SOCK_Acceptor(addr);
	//Because connection request from client is also READ_EVENT,
	//so we register the event for this object to Reactor
	reactor->mRegisterHandler(this, READ_EVENT);
}


/*
 *--------------------------------------------------------------------------------------
 *       Class:  ConnectionAcceptor
 *      Method:  ConnectionAcceptor :: ~ConnectionAcceptor()
 * Description:  Destructor which deregister to Reactor
 *--------------------------------------------------------------------------------------
 */

ConnectionAcceptor::~ConnectionAcceptor(){
	//Remove this handler from Reactor's Demux table
	mReactor->mRemoveHandler(this, READ_EVENT);
	delete mSockAcceptor;
}


/*
 *--------------------------------------------------------------------------------------
 *       Class:  ConnectionAcceptor
 *      Method:  ConnectionAcceptor :: mHandleEvent()
 * Description:  Handler of ConnectionAcceptor class. It listens to request connection
 * 				 from clients.
 *--------------------------------------------------------------------------------------
 */
void ConnectionAcceptor::mHandleEvent(SOCKET h, EventType et){
//	pantheios::log_INFORMATIONAL("Connection request from client !");
	if((et & READ_EVENT) == READ_EVENT){
		//Init new SOCK_Stream with invalid handle.
		//It is FREED in StreamHandler's destructor
		SOCK_Stream* client = new SOCK_Stream();

		//call accept() to accept connect from client
		//and set valid handle for SOCK_Stream
		mSockAcceptor->mAccept(client);
		//Dynamic allocate new StreamHandler object.
		//Pass it created SOCK_Stream object and process-wide Reactor instance
		//It is FREED when client close its connection to server (FIN is sent)
		StreamHandler* handler = new StreamHandler(client, mReactor);
	}
}


/*
 *--------------------------------------------------------------------------------------
 *       Class:  ConnectionAcceptor
 *      Method:  ConnectionAcceptor :: mGetHandle()
 * Description:  Interface to get socket associated with ConnectionAcceptor object
 *--------------------------------------------------------------------------------------
 */
SOCKET ConnectionAcceptor::mGetHandle() const {
	return mSockAcceptor->mGetHandle();
}


/*
 *--------------------------------------------------------------------------------------
 *       Class:  StreamHandler
 *      Method:  StreamHandler :: StreamHandler()
 * Description:  Constructor which register this handler to Reactor.
 *--------------------------------------------------------------------------------------
 */
StreamHandler::StreamHandler(SOCK_Stream* stream, Reactor* reactor){
	//TODO: can we use assignment operator for reference variable
	mSockStream = stream;
	mReactor = reactor;
	reactor->mRegisterHandler(this, READ_EVENT);
	memset(mSipMsg.mBigBuff, 0x00, sizeof(mSipMsg.mBigBuff));
	mSipMsg.mIsReadingBody = false;
	mSipMsg.mRemainBodyLen = 0;
}

/*
 *--------------------------------------------------------------------------------------
 *       Class:  StreamHandler
 *      Method:  StreamHandler :: ~StreamHandler()
 * Description:  Destructor which deregister this handler object to Reactor and remove
 * 				 associated SOCK_Stream object.
 *--------------------------------------------------------------------------------------
 */
StreamHandler::~StreamHandler(){		
	//Remove itself from demultiplexer table of Reactor
	mReactor->mRemoveHandler(this, READ_EVENT);
	//former action requires socket descriptor which get from mSockStream
	//so we remove this SOCK_Stream object latter
	delete mSockStream;
}

/*
 *--------------------------------------------------------------------------------------
 *       Class:  StreamHandler
 *      Method:  StreamHandler :: mHandleEvent()
 * Description:  Handler of StreamHandler class. It processes data sent from clients.
 *--------------------------------------------------------------------------------------
 */
void StreamHandler::mHandleEvent(SOCKET h, EventType et){
//	pantheios::log_INFORMATIONAL("Receiving data from client !");
	if((et & READ_EVENT) == READ_EVENT){
		//save received data to internal buffer
		//then, process it
		mHandleRead(h);
	}else if((et & WRITE_EVENT) == WRITE_EVENT){
		mHandleWrite(h);
	}else if((et & EXCEPT_EVENT) == EXCEPT_EVENT){
		//This object is allocated by ConnectionAcceptor object, so
		//we deallocate it here if event is CLOSE
		mHandleExcept(h);
	}
}

/*
 *--------------------------------------------------------------------------------------
 *       Class:  StreamHandler
 *      Method:  StreamHandler :: mGetHandle()
 * Description:  Interface to get handle associated with a particular StreamHandler object
 *--------------------------------------------------------------------------------------
 */
SOCKET StreamHandler::mGetHandle() const {
	return mSockStream->mGetHandle();
}

/*
 *--------------------------------------------------------------------------------------
 *       Class:  StreamHandler
 *      Method:  StreamHandler :: mHandleRead()
 * Description:  In this function, we will read message from clients and 
 *  			 call to user callback. And in case of TCP, we need to read 
 *  			 until to delimiter of data stream.
 *--------------------------------------------------------------------------------------
 */
void StreamHandler::mHandleRead(SOCKET handle){
	ssize_t n;
	int contlen_value;
	bool isValid = false;		//Check Content-Length value is valid or not
	char buff[TEMP_MSG_SIZE];	//buffer for each time read from kernel
	char* emptyline, *contlen_begin, *contlen_end;
	memset(buff, 0x00, sizeof(buff));

	/////////////////////////////////////////////////////////
	//// We've already read headers. Now we are reading body.
	//// Read until the end of body, save to big buff and
	//// return to user's callback function.
	/////////////////////////////////////////////////////////
	if(mSipMsg.mIsReadingBody == true){
		if(mSipMsg.mRemainBodyLen > TEMP_MSG_SIZE)
			n = read(handle, buff, sizeof(buff));
		else
			n = read(handle, buff, mSipMsg.mRemainBodyLen);

		if((n<0 && errno==ECONNRESET) || (n==0)){
			mHandleClose(handle);
			return;
		}
		if(n < mSipMsg.mRemainBodyLen)
			mSipMsg.mRemainBodyLen = mSipMsg.mRemainBodyLen - n;
		strncat(mSipMsg.mBigBuff, buff, strlen(buff));

		//Call to user's callback function
		//mReactor->mGetReactorImpl()->mTCPReadHandler(handle, mSipMsg.mBigBuff, strlen(mSipMsg.mBigBuff));
		mReactor->mTCPReadHandler(handle, mSipMsg.mBigBuff, strlen(mSipMsg.mBigBuff));

		memset(mSipMsg.mBigBuff, 0x00, sizeof(mSipMsg.mBigBuff));
		mSipMsg.mIsReadingBody = false;
		mSipMsg.mRemainBodyLen = 0;

	//////////////////////////////////////////////////////////////
	//// We are still reading headers. If we found empty line,
	//// save these header bytes and move to read body. Else,
	//// save all read header bytes to big buff and continue read
	//////////////////////////////////////////////////////////////
	}else if(mSipMsg.mIsReadingBody == false){
		n = read(handle, buff, sizeof(buff));
		if((n<0 && errno==ECONNRESET) || (n==0)){
			mHandleClose(handle);
			return;
		}
		if((emptyline = strstr(buff, "\r\n\r\n")) != NULL){
//			pantheios::log_INFORMATIONAL("Found empty line");
			mSipMsg.mIsReadingBody = true;
			int m = (emptyline+4-&buff[0]);
			strncat(mSipMsg.mBigBuff, buff, m);
			int l = strlen(mSipMsg.mBigBuff);
			char inter[l+1];
			strncpy(inter, mSipMsg.mBigBuff, l);
			inter[l+1] = '\0';

			for(int j=0; j<l; j++)
				inter[j] = tolower(inter[j]);
				
			if((contlen_begin = strstr(inter, "content-length")) == NULL){
				if((contlen_begin = strstr(inter, "\r\nl")) == NULL){
//					pantheios::log_INFORMATIONAL("Not found Content-Length header");
					mHandleClose(handle);
					return;
				}
			}

			//pantheios::log_INFORMATIONAL("Found Content-Length");
			if((contlen_end = strstr(contlen_begin, "\r\n")) != NULL){
				for(int i=0; i<((contlen_end-contlen_begin)/sizeof(char)); i++){
					if(isdigit(*(contlen_begin + i))){
						contlen_value = atoi(contlen_begin + i);
						isValid = true;
						break;
					}					
				}
				if(isValid == false) {
					//handle invalid Content-Length value case
					//pantheios::log_INFORMATIONAL("Invalid Content Length !!!");
					mHandleClose(handle);
					return;
				}
			}
			//pantheios::log_INFORMATIONAL("Content-Length: ", pantheios::integer(contlen_value));

			//the number of body bytes in buff[] now is (n-m) bytes.
			//n is the number of read bytes, m is the number of bytes til to end of empty line
			//NOTE: buff[] can also contain a beginning of subsequent message with current message
			//we are reading. So we need to save this message for further reading.
			if(contlen_value > (n-m)){
				strncat(mSipMsg.mBigBuff, emptyline+4, (n-m));
				mSipMsg.mRemainBodyLen = mSipMsg.mRemainBodyLen - (n-m);
			}else{
				//if (n-m) > contlen_value, it means that there are bytes of other
				//message in buff[], we need to save this bytes for that message.
				strncat(mSipMsg.mBigBuff, emptyline+4, contlen_value);

				//Call to user's callback function
				//mReactor->mGetReactorImpl()->mTCPReadHandler(handle, mSipMsg.mBigBuff, strlen(mSipMsg.mBigBuff));
				mReactor->mTCPReadHandler(handle, mSipMsg.mBigBuff, strlen(mSipMsg.mBigBuff));

				memset(mSipMsg.mBigBuff, 0x00, sizeof(mSipMsg.mBigBuff));
				mSipMsg.mIsReadingBody = false;
				mSipMsg.mRemainBodyLen = 0;
			}
		}else{
			//Empty line is not present in buff[]. Save buff to mBigBuff
			strncat(mSipMsg.mBigBuff, buff, n);
		}
	}
}


/*
 *--------------------------------------------------------------------------------------
 *       Class:  StreamHandler
 *      Method:  StreamHandler :: mHandleWrite()
 * Description:  Handler for ready-to-write event
 *--------------------------------------------------------------------------------------
 */

void StreamHandler::mHandleWrite(SOCKET handle){
	//we can ignore this event because we just register READ_EVENT to Reactor
}

/*
 *--------------------------------------------------------------------------------------
 *       Class:  StreamHandler
 *      Method:  StreamHandler :: mHandleClose()
 * Description:  Called by mHandleRead() when it is needed to close this socket.
 *--------------------------------------------------------------------------------------
 */
void StreamHandler::mHandleClose(SOCKET handle){
	//delete this makes destructor is called and this object is freed
	//
	//NOTE: Do not use "delete this" for class that can be instantiated
	//in stack because it is automatic variable and when program go out
	//of its scope, its destructor is call second time. In that case,
	//the result is undefined and may be dangerous.
	delete this;
}

/*
 *--------------------------------------------------------------------------------------
 *       Class:  StreamHandler
 *      Method:  StreamHandler :: mHandleExcept()
 * Description:  Handle other events
 *--------------------------------------------------------------------------------------
 */
void StreamHandler::mHandleExcept(SOCKET handle){
	//we can ignore this event because we just register READ_EVENT to Reactor
}



/*
 *--------------------------------------------------------------------------------------
 *       Class:  DgramHandler
 *      Method:  DgramHandler :: DgramHandler()
 * Description:  
 *--------------------------------------------------------------------------------------
 */
DgramHandler::DgramHandler(const INET_Addr& addr, Reactor* reactor){
	mSockDgram = new SOCK_Datagram(addr);
	mReactor = reactor;
	reactor->mRegisterHandler(this, READ_EVENT);
}

DgramHandler::~DgramHandler(){
	//Removing handler need to get socket descriptor from mSockDgram,
	//so we must delete mSockDgram after calling mRemoveHandler.
	mReactor->mRemoveHandler(this, READ_EVENT);
	delete mSockDgram;
}

void DgramHandler::mHandleEvent(SOCKET sockfd, EventType et){
	if((et & READ_EVENT) == READ_EVENT){
		mHandleRead(sockfd);
	}else if((et & WRITE_EVENT) == WRITE_EVENT){
		mHandleWrite(sockfd);
	}else if((et & EXCEPT_EVENT) == EXCEPT_EVENT){
		mHandleExcept(sockfd);
	}
}

void DgramHandler::mHandleRead(SOCKET sockfd){
	char buff[SIP_UDP_MSG_MAX_SIZE];
	ssize_t n;
	struct sockaddr_in cliaddr;
	socklen_t clilen = sizeof(cliaddr);

	if((n=mSockDgram->mRecvfrom(buff, sizeof(buff), 0, (struct sockaddr*)&cliaddr, &clilen)) < 0){
//		pantheios::log_INFORMATIONAL("Get UDP message fail !!!");
		return;
	}

	//check whether end-of-message reach or not. If not reach, may be message is larger than
	//3kB. We send error response in this case. If reach end of msg, transfer to user's callback
	mReactor->mUDPReadHandler(cliaddr, buff, n);
}

void DgramHandler::mHandleWrite(SOCKET sockfd){
	//We can ignore this method because we just register READ_EVENT to Reactor
}

void DgramHandler::mHandleExcept(SOCKET sockfd){
	//We can ignore this method because we just register READ_EVENT to Reactor
}

SOCKET DgramHandler::mGetHandle() const{
	return mSockDgram->mGetHandle();
}



Reactor* Reactor::sReactor = NULL;

ReactorImpl* Reactor::sReactorImpl = NULL;

/*
 *--------------------------------------------------------------------------------------
 *       Class:  Reactor
 *      Method:  Reactor :: mRegisterHandler()
 * Description:  It just transfers the call to a concrete implementation of Reactor.
 *--------------------------------------------------------------------------------------
 */
void Reactor::mRegisterHandler(EventHandler* eh, EventType et){
	sReactorImpl->mRegisterHandler(eh, et);
}

void Reactor::mRegisterHandler(SOCKET h, EventHandler* eh, EventType et){
	sReactorImpl->mRegisterHandler(h, eh, et);
}

void Reactor::mRemoveHandler(EventHandler* eh, EventType et){
	sReactorImpl->mRemoveHandler(eh, et);
}

void Reactor::mRemoveHandler(SOCKET h, EventType et){
	sReactorImpl->mRemoveHandler(h, et);
}

ReactorImpl* Reactor::mGetReactorImpl(){
	return sReactorImpl;
}

/*
 *--------------------------------------------------------------------------------------
 *       Class:  Reactor
 *      Method:  Reactor :: mHandleEvents()
 * Description:  Call to demultiplexer to wait for events.
 *--------------------------------------------------------------------------------------
 */
void Reactor::mHandleEvents(Time_Value* timeout){
	sReactorImpl->mHandleEvents(timeout);
}


/*
 *--------------------------------------------------------------------------------------
 *       Class:  Reactor
 *      Method:  Reactor :: sInstance()
 * Description:  Access point for user to get solely instance of this class
 *       Usage:  At first time calling, we need to explicitly indicate which 
 *  			 demultiplexer will be used if it is other than SELECT_DEMUX type. 
 *  			 A new ReactorImpl and Reactor object are created.
 *  			 In subsequent calls, we don't need to indicate demultiplexing type. 
 *  			 It just return pointer to solely Reactor instance for caller.
 *--------------------------------------------------------------------------------------
 */
Reactor* Reactor::sInstance(DemuxType demux){
	if(sReactor == NULL){
		switch (demux){
			case SELECT_DEMUX:
				sReactorImpl = new SelectReactorImpl();
				break;
			case POLL_DEMUX:
				sReactorImpl = new PollReactorImpl();
				break;

#ifdef HAS_DEV_POLL
			case DEVPOLL_DEMUX:
				sReactorImpl = new DevPollReactorImpl();
				break;
#endif

#ifdef HAS_EPOLL
			case EPOLL_DEMUX:
				sReactorImpl = new EpollReactorImpl();
				break;
#endif
        
#ifdef HAS_KQUEUE
			case KQUEUE_DEMUX:
				sReactorImpl = new KqueueReactorImpl();
				break;
#endif

			default: break;
		}

		//use "lazy" initialization to sReactor
		//new object is just created at first time calling of sInstance()
		sReactor = new Reactor();
	}
	return sReactor;
}

/*
 *--------------------------------------------------------------------------------------
 *       Class:  Reactor
 *      Method:  Reactor :: Reactor()
 * Description:  Constructor is protected so it assures that there is only instance of
 * 				 Reactor ever created.
 *--------------------------------------------------------------------------------------
 */
Reactor::Reactor(){
	mTCPReadHandler = NULL;
	mTCPEventHandler = NULL;
	mUDPReadHandler = NULL;
	mUDPEventHandler = NULL;
}

Reactor::~Reactor(){
	delete sReactorImpl;
}
