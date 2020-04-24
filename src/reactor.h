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
 *        Class:  Reactor
 *  Description:  Reactor is in charge of demultiplexing and dispatching events
 * =====================================================================================
 */

/**
 * @class Reactor
 *
 * @brief Demultiplex and dispatch events to concrete event handlers.
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


#endif // REACTOR_H_
