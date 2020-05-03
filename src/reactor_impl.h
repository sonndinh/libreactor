#ifndef REACTOR_IMPL_H_
#define REACTOR_IMPL_H_

#include <stdio.h>
#include <poll.h>
#include <cstddef>

#if defined (HAS_DEV_POLL)
#include <sys/devpoll.h>
#endif // HAS_DEV_POLL

#if defined (HAS_EPOLL)
#include <sys/epoll.h>
#endif // HAS_EPOLL

#if defined (HAS_KQUEUE)
#include <sys/event.h>
#include <sys/types.h>
#endif // HAS_KQUEUE

#include "socket_wf.h"
#include "common.h"
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


/**
 * @brief Demultiplexing table contains mapping tuples
 * <SOCKET, EventHandler, EventType>. This table is maintained by each
 * Reactor implementation to dispatch events to correct handlers.
 */
class DemuxTable {
public:
  DemuxTable();
  ~DemuxTable();
  void convert_to_fd_sets(fd_set &readset, fd_set &writeset, fd_set &exceptset, Socket &max_handle);
    
public:
  // The maximum number of file descriptors for select() is FD_SETSIZE.
  struct Tuple table_[FD_SETSIZE];
};


/**
 * @brief Interface for Reactor implementations.
 * User MUST register ReactorStreamHandleRead if they use TCP.
 * User MUST register ReactorDgramHandleRead if they use UDP.
 * ReactorStreamHandleEvent & ReactorDgramHandleEvent can be registered as needed.
 */
class ReactorImpl {
public:
  virtual void register_handler(EventHandler* eh, EventType et) = 0;
  virtual void register_handler(Socket h, EventHandler* eh, EventType et) = 0;
  virtual void remove_handler(EventHandler* eh, EventType et) = 0;
  virtual void remove_handler(Socket h, EventType et) = 0;
  virtual void handle_events(TimeValue* timeout=nullptr) = 0;
};

/**
 * @brief Use select() to demultiplex.
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


/**
 * @brief Use poll() to demultiplex.
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

/**
 * @brief Use /dev/poll as demultiplexer. This class is used with Solaris OS.
 * Handler of file descriptor x is handler_[x].
 * buf_ is an array contains a list of pollfd struct observed.
 * output_ is output of ioctl() function, containing a list of pollfds
 * on which events occur.
 */
#if defined (HAS_DEV_POLL)
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

/**
 * @brief Use Linux's epoll() as demultiplexer.
 */
#if defined (HAS_EPOLL)
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
  struct epoll_event events_[MAXFD]; // Output from epoll_wait()
  EventHandler* handler_[MAXFD];
};

#endif // HAS_EPOLL

/**
 * @brief Use kqueue() (for BSD systems) as demultiplexer.
 */
#if defined (HAS_KQUEUE)
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
  int kqueue_; // Kqueue identifier
  int events_no_; // The number of descriptors we are expecting events occur on.
};

#endif // HAS_KQUEUE

#endif // REACTOR_IMPL_H_
