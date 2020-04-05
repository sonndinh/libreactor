/*
 * =====================================================================================
 *  FILENAME  :  Reactor.c
 *  DESCRIPTION :  
 *  VERSION   :  1.0
 *  CREATED   :  12/03/2010 02:22:17 PM
 *  REVISION  :  none
 *  COMPILER  :  g++
 *  AUTHOR    :  Ngoc Son
 *  COPYRIGHT :  Copyright (c) 2010, Ngoc Son
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
ConnectionAcceptor::ConnectionAcceptor(const InetAddr &addr, Reactor* reactor){
  reactor_ = reactor;
  sock_acceptor_ = new SockAcceptor(addr);
  //Because connection request from client is also READ_EVENT,
  //so we register the event for this object to Reactor
  reactor->register_handler(this, READ_EVENT);
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
  reactor_->remove_handler(this, READ_EVENT);
  delete sock_acceptor_;
}


/*
 *--------------------------------------------------------------------------------------
 *       Class:  ConnectionAcceptor
 *      Method:  ConnectionAcceptor :: mHandleEvent()
 * Description:  Handler of ConnectionAcceptor class. It listens to request connection
 *         from clients.
 *--------------------------------------------------------------------------------------
 */
void ConnectionAcceptor::handle_event(Socket h, EventType et){
  //  pantheios::log_INFORMATIONAL("Connection request from client !");
  if((et & READ_EVENT) == READ_EVENT){
    //Init new SOCK_Stream with invalid handle.
    //It is FREED in StreamHandler's destructor
    SockStream* client = new SockStream();

    //call accept() to accept connect from client
    //and set valid handle for SOCK_Stream
    sock_acceptor_->accept_sock(client);
    //Dynamic allocate new StreamHandler object.
    //Pass it created SOCK_Stream object and process-wide Reactor instance
    //It is FREED when client close its connection to server (FIN is sent)
    StreamHandler* handler = new StreamHandler(client, reactor_);
  }
}


/*
 *--------------------------------------------------------------------------------------
 *       Class:  ConnectionAcceptor
 *      Method:  ConnectionAcceptor :: mGetHandle()
 * Description:  Interface to get socket associated with ConnectionAcceptor object
 *--------------------------------------------------------------------------------------
 */
Socket ConnectionAcceptor::get_handle() const {
  return sock_acceptor_->get_handle();
}


/*
 *--------------------------------------------------------------------------------------
 *       Class:  StreamHandler
 *      Method:  StreamHandler :: StreamHandler()
 * Description:  Constructor which register this handler to Reactor.
 *--------------------------------------------------------------------------------------
 */
StreamHandler::StreamHandler(SockStream* stream, Reactor* reactor){
  //TODO: can we use assignment operator for reference variable
  sock_stream_ = stream;
  reactor_ = reactor;
  reactor->register_handler(this, READ_EVENT);
  
  memset(sip_msg_.big_buff, 0x00, sizeof(sip_msg_.big_buff));
  sip_msg_.is_reading_body = false;
  sip_msg_.remain_body_len = 0;
}

/*
 *--------------------------------------------------------------------------------------
 *       Class:  StreamHandler
 *      Method:  StreamHandler :: ~StreamHandler()
 * Description:  Destructor which deregister this handler object to Reactor and remove
 *         associated SOCK_Stream object.
 *--------------------------------------------------------------------------------------
 */
StreamHandler::~StreamHandler(){    
  //Remove itself from demultiplexer table of Reactor
  reactor_->remove_handler(this, READ_EVENT);
  //former action requires socket descriptor which get from mSockStream
  //so we remove this SOCK_Stream object latter
  delete sock_stream_;
}

/*
 *--------------------------------------------------------------------------------------
 *       Class:  StreamHandler
 *      Method:  StreamHandler :: mHandleEvent()
 * Description:  Handler of StreamHandler class. It processes data sent from clients.
 *--------------------------------------------------------------------------------------
 */
void StreamHandler::handle_event(Socket h, EventType et){
  //  pantheios::log_INFORMATIONAL("Receiving data from client !");
  if((et & READ_EVENT) == READ_EVENT){
    //save received data to internal buffer
    //then, process it
    handle_read(h);
  }else if((et & WRITE_EVENT) == WRITE_EVENT){
    handle_write(h);
  }else if((et & EXCEPT_EVENT) == EXCEPT_EVENT){
    //This object is allocated by ConnectionAcceptor object, so
    //we deallocate it here if event is CLOSE
    handle_except(h);
  }
}

/*
 *--------------------------------------------------------------------------------------
 *       Class:  StreamHandler
 *      Method:  StreamHandler :: mGetHandle()
 * Description:  Interface to get handle associated with a particular StreamHandler object
 *--------------------------------------------------------------------------------------
 */
Socket StreamHandler::get_handle() const {
  return sock_stream_->get_handle();
}

/*
 *--------------------------------------------------------------------------------------
 *       Class:  StreamHandler
 *      Method:  StreamHandler :: mHandleRead()
 * Description:  In this function, we will read message from clients and 
 *         call to user callback. And in case of TCP, we need to read 
 *         until to delimiter of data stream.
 *--------------------------------------------------------------------------------------
 */
void StreamHandler::handle_read(Socket handle){
  ssize_t n;
  int contlen_value;
  bool is_valid = false;   //Check Content-Length value is valid or not
  char buff[TEMP_MSG_SIZE]; //buffer for each time read from kernel
  char* emptyline, *contlen_begin, *contlen_end;
  memset(buff, 0x00, sizeof(buff));

  /////////////////////////////////////////////////////////
  //// We've already read headers. Now we are reading body.
  //// Read until the end of body, save to big buff and
  //// return to user's callback function.
  /////////////////////////////////////////////////////////
  if(sip_msg_.is_reading_body == true){
    if(sip_msg_.remain_body_len > TEMP_MSG_SIZE)
      n = read(handle, buff, sizeof(buff));
    else
      n = read(handle, buff, sip_msg_.remain_body_len);

    if((n < 0 && errno == ECONNRESET) || (n == 0)) {
      handle_close(handle);
      return;
    }
    if(n < sip_msg_.remain_body_len)
      sip_msg_.remain_body_len = sip_msg_.remain_body_len - n;
    strncat(sip_msg_.big_buff, buff, strlen(buff));

    //Call to user's callback function
    //mReactor->mGetReactorImpl()->mTCPReadHandler(handle, mSipMsg.mBigBuff, strlen(mSipMsg.mBigBuff));
    reactor_->tcp_read_handler_(handle, sip_msg_.big_buff, strlen(sip_msg_.big_buff));

    memset(sip_msg_.big_buff, 0x00, sizeof(sip_msg_.big_buff));
    sip_msg_.is_reading_body = false;
    sip_msg_.remain_body_len = 0;

    //////////////////////////////////////////////////////////////
    //// We are still reading headers. If we found empty line,
    //// save these header bytes and move to read body. Else,
    //// save all read header bytes to big buff and continue read
    //////////////////////////////////////////////////////////////
  }else if(sip_msg_.is_reading_body == false){
    n = read(handle, buff, sizeof(buff));
    if((n < 0 && errno == ECONNRESET) || (n == 0)){
      handle_close(handle);
      return;
    }
    if((emptyline = strstr(buff, "\r\n\r\n")) != NULL){
      //      pantheios::log_INFORMATIONAL("Found empty line");
      sip_msg_.is_reading_body = true;
      int m = (emptyline + 4 - &buff[0]);
      strncat(sip_msg_.big_buff, buff, m);
      int l = strlen(sip_msg_.big_buff);
      char inter[l+1];
      strncpy(inter, sip_msg_.big_buff, l);
      inter[l+1] = '\0';

      for(int j = 0; j < l; j++)
        inter[j] = tolower(inter[j]);
        
      if((contlen_begin = strstr(inter, "content-length")) == NULL){
        if((contlen_begin = strstr(inter, "\r\nl")) == NULL){
          //          pantheios::log_INFORMATIONAL("Not found Content-Length header");
          handle_close(handle);
          return;
        }
      }

      //pantheios::log_INFORMATIONAL("Found Content-Length");
      if((contlen_end = strstr(contlen_begin, "\r\n")) != NULL){
        for(int i = 0; i < ((contlen_end-contlen_begin)/sizeof(char)); i++){
          if(isdigit(*(contlen_begin + i))){
            contlen_value = atoi(contlen_begin + i);
            is_valid = true;
            break;
          }         
        }
        if(is_valid == false) {
          //handle invalid Content-Length value case
          //pantheios::log_INFORMATIONAL("Invalid Content Length !!!");
          handle_close(handle);
          return;
        }
      }
      //pantheios::log_INFORMATIONAL("Content-Length: ", pantheios::integer(contlen_value));

      //the number of body bytes in buff[] now is (n-m) bytes.
      //n is the number of read bytes, m is the number of bytes til to end of empty line
      //NOTE: buff[] can also contain a beginning of subsequent message with current message
      //we are reading. So we need to save this message for further reading.
      if(contlen_value > (n-m)){
        strncat(sip_msg_.big_buff, emptyline+4, (n-m));
        sip_msg_.remain_body_len = sip_msg_.remain_body_len - (n-m);
      }else{
        //if (n-m) > contlen_value, it means that there are bytes of other
        //message in buff[], we need to save this bytes for that message.
        strncat(sip_msg_.big_buff, emptyline+4, contlen_value);

        //Call to user's callback function
        //mReactor->mGetReactorImpl()->mTCPReadHandler(handle, mSipMsg.mBigBuff, strlen(mSipMsg.mBigBuff));
        reactor_->tcp_read_handler_(handle, sip_msg_.big_buff, strlen(sip_msg_.big_buff));

        memset(sip_msg_.big_buff, 0x00, sizeof(sip_msg_.big_buff));
        sip_msg_.is_reading_body = false;
        sip_msg_.remain_body_len = 0;
      }
    }else{
      //Empty line is not present in buff[]. Save buff to mBigBuff
      strncat(sip_msg_.big_buff, buff, n);
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

void StreamHandler::handle_write(Socket handle){
  //we can ignore this event because we just register READ_EVENT to Reactor
}

/*
 *--------------------------------------------------------------------------------------
 *       Class:  StreamHandler
 *      Method:  StreamHandler :: mHandleClose()
 * Description:  Called by mHandleRead() when it is needed to close this socket.
 *--------------------------------------------------------------------------------------
 */
void StreamHandler::handle_close(Socket handle){
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
void StreamHandler::handle_except(Socket handle){
  //we can ignore this event because we just register READ_EVENT to Reactor
}



/*
 *--------------------------------------------------------------------------------------
 *       Class:  DgramHandler
 *      Method:  DgramHandler :: DgramHandler()
 * Description:  
 *--------------------------------------------------------------------------------------
 */
DgramHandler::DgramHandler(const InetAddr& addr, Reactor* reactor){
  sock_dgram_ = new SockDatagram(addr);
  reactor_ = reactor;
  reactor->register_handler(this, READ_EVENT);
}

DgramHandler::~DgramHandler(){
  //Removing handler need to get socket descriptor from mSockDgram,
  //so we must delete mSockDgram after calling mRemoveHandler.
  reactor_->remove_handler(this, READ_EVENT);
  delete sock_dgram_;
}

void DgramHandler::handle_event(Socket sockfd, EventType et){
  if((et & READ_EVENT) == READ_EVENT){
    handle_read(sockfd);
  }else if((et & WRITE_EVENT) == WRITE_EVENT){
    handle_write(sockfd);
  }else if((et & EXCEPT_EVENT) == EXCEPT_EVENT){
    handle_except(sockfd);
  }
}

void DgramHandler::handle_read(Socket sockfd){
  char buff[SIP_UDP_MSG_MAX_SIZE];
  ssize_t n;
  struct sockaddr_in cliaddr;
  socklen_t clilen = sizeof(cliaddr);

  n = sock_dgram_->recv_from(buff, sizeof(buff), 0, (struct sockaddr*)&cliaddr, &clilen);
  if(n < 0){
    //    pantheios::log_INFORMATIONAL("Get UDP message fail !!!");
    return;
  }

  //check whether end-of-message reach or not. If not reach, may be message is larger than
  //3kB. We send error response in this case. If reach end of msg, transfer to user's callback
  reactor_->udp_read_handler_(cliaddr, buff, n);
}

void DgramHandler::handle_write(Socket sockfd){
  //We can ignore this method because we just register READ_EVENT to Reactor
}

void DgramHandler::handle_except(Socket sockfd){
  //We can ignore this method because we just register READ_EVENT to Reactor
}

Socket DgramHandler::get_handle() const{
  return sock_dgram_->get_handle();
}

Reactor* Reactor::reactor_ = nullptr;

ReactorImpl* Reactor::reactor_impl_ = nullptr;

/*
 *--------------------------------------------------------------------------------------
 *       Class:  Reactor
 *      Method:  Reactor :: mRegisterHandler()
 * Description:  It just transfers the call to a concrete implementation of Reactor.
 *--------------------------------------------------------------------------------------
 */
void Reactor::register_handler(EventHandler* eh, EventType et){
  reactor_impl_->register_handler(eh, et);
}

void Reactor::register_handler(Socket h, EventHandler* eh, EventType et){
  reactor_impl_->register_handler(h, eh, et);
}

void Reactor::remove_handler(EventHandler* eh, EventType et){
  reactor_impl_->remove_handler(eh, et);
}

void Reactor::remove_handler(Socket h, EventType et){
  reactor_impl_->remove_handler(h, et);
}

ReactorImpl* Reactor::get_reactor_impl(){
  return reactor_impl_;
}

/*
 *--------------------------------------------------------------------------------------
 *       Class:  Reactor
 *      Method:  Reactor :: mHandleEvents()
 * Description:  Call to demultiplexer to wait for events.
 *--------------------------------------------------------------------------------------
 */
void Reactor::handle_events(TimeValue* timeout){
  reactor_impl_->handle_events(timeout);
}


/*
 *--------------------------------------------------------------------------------------
 *       Class:  Reactor
 *      Method:  Reactor :: sInstance()
 * Description:  Access point for user to get solely instance of this class
 *       Usage:  At first time calling, we need to explicitly indicate which 
 *         demultiplexer will be used if it is other than SELECT_DEMUX type. 
 *         A new ReactorImpl and Reactor object are created.
 *         In subsequent calls, we don't need to indicate demultiplexing type. 
 *         It just return pointer to solely Reactor instance for caller.
 *--------------------------------------------------------------------------------------
 */
Reactor* Reactor::instance(DemuxType demux){
  if(reactor_ == nullptr){
    switch (demux){
    case SELECT_DEMUX:
      reactor_impl_ = new SelectReactorImpl();
      break;
    case POLL_DEMUX:
      reactor_impl_ = new PollReactorImpl();
      break;

#ifdef HAS_DEV_POLL
    case DEVPOLL_DEMUX:
      reactor_impl_ = new DevPollReactorImpl();
      break;
#endif
      
#ifdef HAS_EPOLL
    case EPOLL_DEMUX:
      reactor_impl_ = new EpollReactorImpl();
      break;
#endif
      
#ifdef HAS_KQUEUE
    case KQUEUE_DEMUX:
      reactor_impl_ = new KqueueReactorImpl();
      break;
#endif
      
    default: break;
    }

    //use "lazy" initialization to sReactor
    //new object is just created at first time calling of sInstance()
    reactor_ = new Reactor();
  }
  return reactor_;
}

/*
 *--------------------------------------------------------------------------------------
 *       Class:  Reactor
 *      Method:  Reactor :: Reactor()
 * Description:  Constructor is protected so it assures that there is only instance of
 *         Reactor ever created.
 *--------------------------------------------------------------------------------------
 */
Reactor::Reactor(){
  tcp_read_handler_ = nullptr;
  tcp_event_handler_ = nullptr;
  udp_read_handler_ = nullptr;
  udp_event_handler_ = nullptr;
}

Reactor::~Reactor(){
  delete reactor_impl_;
}
