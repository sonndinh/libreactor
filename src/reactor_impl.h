/*
 * =====================================================================================
 *  FILENAME  :  ReactorImpl.h
 *  DESCRIPTION :  
 *  VERSION   :  1.0
 *  CREATED   :  12/03/2010 02:24:11 PM
 *  REVISION  :  none
 *  COMPILER  :  g++
 *  AUTHOR    :  Ngoc Son
 *  COPYRIGHT :  Copyright (c) 2010, Ngoc Son
 *
 * =====================================================================================
 */
#ifndef REACTOR_IMPL_H_
#define REACTOR_IMPL_H_

#include <stdio.h>
#include <poll.h>
#include <cstddef>

#include "socket_wf.h"
#include "reactor_type.h"
#include "reactor.h"

class EventHandler;

struct Tuple {
  //pointer to Event_Handler that processes the events arriving
  //on the handle
  EventHandler* event_handler;
  //bitmask that tracks which types of events <Event_Handler>
  //is registered for
  EventType event_type;
};


/*
 * =====================================================================================
 *        Class:  DemuxTable
 *  Description:  Demultiplexing table contains mapping tuples
 *          <SOCKET, EventHandler, EventType>. This table maintained by
 *          implementing Reactor class so it can dispatch current event with a
 *          particular type to appropriate handler.
 * =====================================================================================
 */
class DemuxTable {
public:
  DemuxTable();
  ~DemuxTable();
  void convert_to_fd_sets(fd_set &readset, fd_set &writeset, fd_set &exceptset, Socket &max_handle);
    
public:
  //because the number of file descriptors can be demultiplexed by
  //select() is limited by FD_SETSIZE constant so this table is indexed
  //up to FD_SETSIZE
  struct Tuple table_[FD_SETSIZE];
};


/*
 * =====================================================================================
 *        Class:  ReactorImpl
 *  Description:  Interface for implementation of Reactor. It is abstract base class.
 *          User MUST register ReactorStreamHandleRead if they use TCP
 *          User MUST register ReactorDgramHandleRead if they use UDP
 *          ReactorStreamHandleEvent & ReactorDgramHandleEvent can be registered
 *          as user's demand.
 *       Note:  That this pure abstract class just define interfaces for transfering 
 *            read data to callbacks. Transfer event methods to user's callbacks can 
 *            be implemented by derived classes such as SelectReactorImpl,...
 * =====================================================================================
 */
class ReactorImpl {
public:
  virtual void register_handler(EventHandler* eh, EventType et)=0;
  virtual void register_handler(Socket h, EventHandler* eh, EventType et)=0;
  virtual void remove_handler(EventHandler* eh, EventType et)=0;
  virtual void remove_handler(Socket h, EventType et)=0;
  virtual void handle_events(TimeValue* timeout=nullptr)=0;
};

////////////////////////////////////////////////////////
//Following classes are concrete classes which implement
//ReactorImpl interface using select(), poll(), epoll() 
//, kqueue and /dev/poll demultiplexer
////////////////////////////////////////////////////////


/*
 * =====================================================================================
 *        Class:  SelectReactorImpl
 *  Description:  It uses traditional select() function as demultiplexer
 * =====================================================================================
 */
class SelectReactorImpl : public ReactorImpl {
public:
  SelectReactorImpl();
  ~SelectReactorImpl();

  void register_handler(EventHandler* eh, EventType et);
  void register_handler(Socket h, EventHandler* eh, EventType et);
  void remove_handler(EventHandler* eh, EventType et);
  void remove_handler(Socket h, EventType et);
  void handle_events(TimeValue* timeout=nullptr);

private:
  DemuxTable table_;
  fd_set  rdset_, wrset_, exset_;
  int   max_handle_;
};


/*
 * =====================================================================================
 *        Class:  PollReactorImpl
 *  Description:  It uses traditional poll() function as demultiplexer
 * =====================================================================================
 */
class PollReactorImpl : public ReactorImpl {
public:
  PollReactorImpl();
  ~PollReactorImpl();

  void register_handler(EventHandler* eh, EventType et);
  void register_handler(Socket h, EventHandler* eh, EventType et);
  void remove_handler(EventHandler* eh, EventType et);
  void remove_handler(Socket h, EventType et);
  void handle_events(TimeValue* timeout=nullptr);

private:
  struct pollfd client_[MAXFD];
  EventHandler* handler_[MAXFD];
  int maxi_;
};

/*
 * =====================================================================================
 *        Class:  DevPollReactorImpl
 *  Description:  It uses /dev/poll as demultiplexer. This class is used with Solaris OS.
 *  NOTE     :  - handler of file descriptor x is mHandler[x]
 *          - mBuf[] is array contains list of pollfd struct are currently observed
 *          - mOutput is output of ioctl() function, contains list of pollfds
 *          on which events occur.
 * =====================================================================================
 */
#ifdef HAS_DEV_POLL
#include <sys/devpoll.h>

class DevPollReactorImpl : public ReactorImpl {
public:
  DevPollReactorImpl();
  ~DevPollReactorImpl();

  void register_handler(EventHandler* eh, EventType et);
  void register_handler(Socket h, EventHandler* eh, EventType et);
  void remove_handler(EventHandler* eh, EventType et);
  void remove_handler(Socket h, EventType et);
  void handle_events(TimeValue* timeout=nullptr);

private:
  int devpollfd_;
  struct pollfd buf_[MAXFD]; //input interested file descriptors
  struct pollfd* output_;    //file descriptors has event
  EventHandler* handler_[MAXFD]; //keep track of handler for each file descriptor
};

#endif // HAS_DEV_POLL

/*
 * =====================================================================================
 *        Class:  EpollReactorImpl
 *  Description:  It uses Linux's epoll() as demultiplexer
 * =====================================================================================
 */
#ifdef HAS_EPOLL
#include <sys/epoll.h>

class EpollReactorImpl : public ReactorImpl {
public:
  EpollReactorImpl();
  ~EpollReactorImpl();

  void register_handler(EventHandler* eh, EventType et);
  void register_handler(Socket h, EventHandler* eh, EventType et);
  void remove_handler(EventHandler* eh, EventType et);
  void remove_handler(Socket h, EventType et);
  void handle_events(TimeValue* timeout=nullptr);

private:
  int epollfd_;
  struct epoll_event events_[MAXFD];//Output from epoll_wait()
  EventHandler* handler_[MAXFD];
};

#endif // HAS_EPOLL

/*
 * =====================================================================================
 *        Class:  KqueueReactorImpl
 *  Description:  It uses BSD-derived kqueue() as demultiplexer. This class
 *          used for BSD-derived systems such as FreeBSD, NetBSD
 * =====================================================================================
 */
#ifdef HAS_KQUEUE
#include <sys/event.h>
#include <sys/types.h>

class KqueueReactorImpl : public ReactorImpl {
public:
  KqueueReactorImpl();
  ~KqueueReactorImpl();
    
  void register_handler(EventHandler* eh, EventType et);
  void register_handler(Socket h, EventHandler* eh, EventType et);
  void remove_handler(EventHandler* eh, EventType et);
  void remove_handler(Socket h, EventType et);
  void handle_events(TimeValue* timeout=nullptr);

private:
  int kqueue_;  //Kqueue identifier
  int events_no_;  //The number of descriptors we are expecting events occur on.
};

#endif // HAS_KQUEUE

#endif // REACTOR_IMPL_H_
