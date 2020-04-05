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
  virtual void handle_event(Socket handle, EventType et) = 0;
  virtual Socket get_handle() const = 0;
};


/*
 * =====================================================================================
 *        Class:  Reactor
 *  Description:  Reactor is in charge of demultiplexing and dispatching events
 * =====================================================================================
 */
class Reactor {
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

public:
    
  /* 
   * ===  FUNCTION  ======================================================================
   *         Name:  mRegisterTCPCbFuncs()
   *  Description:  Function to register user callbacks for incoming TCP data 
   *          and TCP events.
   * =====================================================================================
   */
  virtual void register_tcp_callbacks(ReactorStreamHandleRead read_cb, ReactorStreamHandleEvent event_cb){
    tcp_read_handler_ = read_cb;
    tcp_event_handler_ = event_cb;
  }

  /* 
   * ===  FUNCTION  ======================================================================
   *         Name:  mRegisterUDPCbFuncs()
   *  Description:  Function to register user callbacks for incoming UDP datagrams
   *          and UDP events.
   * =====================================================================================
   */
  virtual void register_udp_callbacks(ReactorDgramHandleRead read_cb, ReactorDgramHandleEvent event_cb){
    udp_read_handler_ = read_cb;
    udp_event_handler_ = event_cb;
  }

  /* 
   * ===  FUNCTION  ======================================================================
   *         Name:  mRegisterHandler()
   *  Description:  Interface for registering handler
   * =====================================================================================
   */
  virtual void register_handler(EventHandler* eh, EventType et);
  virtual void register_handler(Socket h, EventHandler* eh, EventType et);

  /* 
   * ===  FUNCTION  ======================================================================
   *         Name:  mRemoveHandler()
   *  Description:  Interface for removing handler
   * =====================================================================================
   */
  virtual void remove_handler(EventHandler* eh, EventType et);
  virtual void remove_handler(Socket h, EventType et);

  ReactorImpl* get_reactor_impl();
  
  /* 
   * ===  FUNCTION  ======================================================================
   *         Name:  mHandleEvents()
   *  Description:  Main loop for handling incoming events
   * =====================================================================================
   */
  void handle_events(TimeValue* timeout=nullptr);
  
  /* 
   * ===  FUNCTION  ======================================================================
   *         Name:  sInstance()
   *  Description:  Access point for user to get solely instance of this class
   * =====================================================================================
   */
  static Reactor* instance(DemuxType type=SELECT_DEMUX);
  
  
public:
  ReactorStreamHandleRead   tcp_read_handler_;
  ReactorStreamHandleEvent  tcp_event_handler_;
  ReactorDgramHandleRead    udp_read_handler_;
  ReactorDgramHandleEvent   udp_event_handler_;
  
protected:
  //Pointer to abstract implementation of Reactor
  //using to Bridge pattern
  static ReactorImpl* reactor_impl_;

  //Pointer to a process-wide Reactor singleton
  static Reactor* reactor_;
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
public:
  ConnectionAcceptor(const INET_Addr &addr, Reactor* reactor);
  ~ConnectionAcceptor();

  virtual void handle_event(Socket handle, EventType et);
  virtual Socket get_handle() const;
  
private:
  //Socket factory that accepts client connections
  SOCK_Acceptor* sock_acceptor_;
  
  //Cached Reactor
  Reactor* reactor_;
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
  char big_buff[SIP_MSG_MAX_SIZE];
  bool is_reading_body;
  int remain_body_len;
};

class StreamHandler : public EventHandler {
public:
  StreamHandler(SOCK_Stream* stream, Reactor* reactor);
  ~StreamHandler();
  
  virtual void handle_event(Socket handle, EventType et);
  virtual Socket get_handle() const;

protected:
  virtual void handle_read(Socket handle);
  virtual void handle_write(Socket handle);
  virtual void handle_close(Socket handle);
  virtual void handle_except(Socket handle);

private:
  //Receives data from a connected client
  SOCK_Stream* sock_stream_;
  //Store process-wide Reactor instance
  Reactor* reactor_;
  struct SipMsgBuff sip_msg_;
};


/*
 * =====================================================================================
 *        Class:  DgramHandler
 *  Description:  This class wraps functions for handling UDP messages from clients
 * =====================================================================================
 */
class DgramHandler : public EventHandler {
public: 
  DgramHandler(const INET_Addr& addr, Reactor* reactor);
  ~DgramHandler();
  
  virtual void handle_event(Socket sockfd, EventType et);
  virtual Socket get_handle() const;

protected:
  virtual void handle_read(Socket sockfd);
  virtual void handle_write(Socket sockfd);
  virtual void handle_except(Socket sockfd);

private:
  SOCK_Datagram* sock_dgram_;
  Reactor* reactor_;
};

#endif // REACTOR_H_
