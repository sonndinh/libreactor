/*
 * =====================================================================================
 *  FILENAME  :  Reactor.h
 *  DESCRIPTION :  Reactor class uses Singleton and Bridge pattern to
 *           define abstract interface.
 *  VERSION   :  1.0
 *  CREATED   :  12/03/2010 02:23:12 PM
 *  REVISION  :  none
 *  COMPILER  :  g++
 *  AUTHOR    :  Ngoc Son
 *  COPYRIGHT :  Copyright (c) 2010, Ngoc Son
 *
 * =====================================================================================
 */
#ifndef REACTOR_H_
#define REACTOR_H_

#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <string.h>
#include "socket_wf.h"
#include "reactor_type.h"
#include "reactor_impl.h"

class ReactorImpl;


/*
 * =====================================================================================
 *        Class:  EventHandler
 *  Description:  Following class is single handler methods of Event_Handler.
 *          Method in this class are called when events come. It is abstract
 *          base class for handling events so we cannot instantiate it
 * =====================================================================================
 */
class EventHandler {
public:
  //NOTE: may be we don't need "handle" parameter in mHandleEvent()
  //because when mHandleEvent() called, it can get associated socket
  //descriptor through SOCK_Acceptor or SOCK_Stream or SOCK_Datagram
  virtual void handle_event(SOCKET handle, EventType et) = 0;
  virtual SOCKET get_handle() const = 0;
};


/*
 * =====================================================================================
 *        Class:  Reactor
 *  Description:  Reactor is in charge of demultiplexing and dispatching events
 * =====================================================================================
 */
class Reactor {
public:
  ReactorStreamHandleRead   mTCPReadHandler;
  ReactorStreamHandleEvent  mTCPEventHandler;
  ReactorDgramHandleRead    mUDPReadHandler;
  ReactorDgramHandleEvent   mUDPEventHandler;
    
protected:
  //Pointer to abstract implementation of Reactor
  //using to Bridge pattern
  static ReactorImpl* sReactorImpl;

  //Pointer to a process-wide Reactor singleton
  static Reactor* sReactor;

public:
    
  /* 
   * ===  FUNCTION  ======================================================================
   *         Name:  mRegisterTCPCbFuncs()
   *  Description:  Function to register user callbacks for incoming TCP data 
   *          and TCP events.
   * =====================================================================================
   */
  virtual void mRegisterTCPCbFuncs(ReactorStreamHandleRead readCb, ReactorStreamHandleEvent eventCb){
    mTCPReadHandler = readCb;
    mTCPEventHandler = eventCb;
  }

  /* 
   * ===  FUNCTION  ======================================================================
   *         Name:  mRegisterUDPCbFuncs()
   *  Description:  Function to register user callbacks for incoming UDP datagrams
   *          and UDP events.
   * =====================================================================================
   */
  virtual void mRegisterUDPCbFuncs(ReactorDgramHandleRead readCb, ReactorDgramHandleEvent eventCb){
    mUDPReadHandler = readCb;
    mUDPEventHandler = eventCb;
  }

  /* 
   * ===  FUNCTION  ======================================================================
   *         Name:  mRegisterHandler()
   *  Description:  Interface for registering handler
   * =====================================================================================
   */
  virtual void mRegisterHandler(EventHandler* eh, EventType et);
  virtual void mRegisterHandler(SOCKET h, EventHandler* eh, EventType et);

  /* 
   * ===  FUNCTION  ======================================================================
   *         Name:  mRemoveHandler()
   *  Description:  Interface for removing handler
   * =====================================================================================
   */
  virtual void mRemoveHandler(EventHandler* eh, EventType et);
  virtual void mRemoveHandler(SOCKET h, EventType et);

  ReactorImpl* mGetReactorImpl();
    
  /* 
   * ===  FUNCTION  ======================================================================
   *         Name:  mHandleEvents()
   *  Description:  Main loop for handling incoming events
   * =====================================================================================
   */
  void mHandleEvents(Time_Value* timeout=NULL);

  /* 
   * ===  FUNCTION  ======================================================================
   *         Name:  sInstance()
   *  Description:  Access point for user to get solely instance of this class
   * =====================================================================================
   */
  static Reactor* sInstance(DemuxType type=SELECT_DEMUX);

protected:
  /* 
   * ===  FUNCTION  ======================================================================
   *         Name:  Reactor()
   *  Description:  Construtor is protected so it assures that there is only 
   *          a instance of Reactor ever created.
   * =====================================================================================
   */
  Reactor();
  ~Reactor();
};


/*
 * =====================================================================================
 *        Class:  ConnectionAcceptor
 *  Description:  Implementation class for accepting connection from client.
 *          A ConnectionAcceptor handles all TCP connection requests from clients
 *          through listening TCP socket (SOCK_Acceptor) on a particular address.
 * =====================================================================================
 */
class ConnectionAcceptor : public EventHandler {
private:
  //Socket factory that accepts client connections
  SOCK_Acceptor* mSockAcceptor;

  //Cached Reactor
  Reactor* mReactor;

public:
  ConnectionAcceptor(const INET_Addr &addr, Reactor* reactor);
  ~ConnectionAcceptor();

  virtual void mHandleEvent(SOCKET handle, EventType et);
  virtual SOCKET mGetHandle() const;
};

//SOCK_Acceptor handles factory enables a ConnectionAcceptor object
//to accept connection indications on a passive-mode socket handle that is
//listening on a transport endpoint. When a connection arrives from a client,
//the SOCK_Acceptor accepts the connection passively and produces an initialized
//SOCK_Stream. The SOCK_Stream is then uses TCP to transfer data reliably between 
//the client and the server.


/*
 * =====================================================================================
 *        Class:  StreamHandler
 *  Description:  StreamHandler receives and processes data from clients.
 *          After connection is accepted, StreamHandler is responsible
 *          for handling data from client. Each StreamHandler object is in charge
 *          of handling data stream from a particular TCP connection (SOCK_Stream)
 * =====================================================================================
 */
struct SipMsgBuff{
  char  mBigBuff[SIP_MSG_MAX_SIZE];
  bool  mIsReadingBody;
  int   mRemainBodyLen;
};

class StreamHandler : public EventHandler {
private:
  //Receives data from a connected client
  SOCK_Stream*    mSockStream;
  //Store process-wide Reactor instance
  Reactor*      mReactor;
  struct SipMsgBuff mSipMsg;

public:
  StreamHandler(SOCK_Stream* stream, Reactor* reactor);
  ~StreamHandler();
  virtual void mHandleEvent(SOCKET handle, EventType et);
  virtual SOCKET mGetHandle() const;

protected:
  virtual void mHandleRead(SOCKET handle);
  virtual void mHandleWrite(SOCKET handle);
  virtual void mHandleClose(SOCKET handle);
  virtual void mHandleExcept(SOCKET handle);
};


/*
 * =====================================================================================
 *        Class:  DgramHandler
 *  Description:  This class wraps functions for handling UDP messages from clients
 * =====================================================================================
 */
class DgramHandler : public EventHandler {
private:
  SOCK_Datagram*    mSockDgram;
  Reactor*      mReactor;

public: 
  DgramHandler(const INET_Addr& addr, Reactor* reactor);
  ~DgramHandler();
  virtual void mHandleEvent(SOCKET sockfd, EventType et);
  virtual SOCKET mGetHandle() const;

protected:
  virtual void mHandleRead(SOCKET sockfd);
  virtual void mHandleWrite(SOCKET sockfd);
  virtual void mHandleExcept(SOCKET sockfd);
};
#endif // REACTOR_H_
