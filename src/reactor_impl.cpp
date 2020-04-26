/*
 * =====================================================================================
 *  FILENAME  :  ReactorImpl.c
 *  DESCRIPTION :  
 *  VERSION   :  1.0
 *  CREATED   :  12/03/2010 02:22:46 PM
 *  REVISION  :  none
 *  COMPILER  :  g++
 *  AUTHOR    :  Ngoc Son
 *  COPYRIGHT :  Copyright (c) 2010, Ngoc Son
 *
 * =====================================================================================
 */
#include <iostream>

#include "reactor_impl.h"



/*
 *--------------------------------------------------------------------------------------
 *       Class:  DemuxTable
 *      Method:  DemuxTable :: DemuxTable()
 * Description:  Constructor just inits memory
 *--------------------------------------------------------------------------------------
 */
DemuxTable::DemuxTable(){
  memset(table_, 0x00, FD_SETSIZE * sizeof(struct Tuple));
}

DemuxTable::~DemuxTable(){}

/*
 *--------------------------------------------------------------------------------------
 *       Class:  DemuxTable
 *      Method:  DemuxTable :: mConvertToFdSets()
 * Description:  This method converts array of Tuples to three set of file descriptor
 *         before calling demultiplexing function. It used with select().
 *--------------------------------------------------------------------------------------
 */
void DemuxTable::convert_to_fd_sets(fd_set &readset, fd_set &writeset, fd_set &exceptset, Socket &max_handle){
  for(Socket i = 0; i < FD_SETSIZE; i++){
    if(table_[i].event_handler != nullptr){
      //We are interested in this socket, so
      //set max_handle to this socket descriptor
      max_handle = i;
      if((table_[i].event_type & READ_EVENT) == READ_EVENT)
        FD_SET(i, &readset);
      if((table_[i].event_type & WRITE_EVENT) == WRITE_EVENT)
        FD_SET(i, &writeset);
      if((table_[i].event_type & EXCEPT_EVENT) == EXCEPT_EVENT)
        FD_SET(i, &exceptset);
    }
  }
}



/*
 * =====================================================================================
 *        Class:  EpollReactorImpl
 *  Description:  Reactor implementation using epoll() function.
 * =====================================================================================
 */

/*
 *--------------------------------------------------------------------------------------
 *       Class:  EpollReactorImpl
 *      Method:  EpollReactorImpl :: EpollReactorImpl()
 * Description:  Constructor which create a new epoll instance
 *--------------------------------------------------------------------------------------
 */
#ifdef HAS_EPOLL

EpollReactorImpl::EpollReactorImpl(){
  for(int i = 0; i < MAXFD; i++){
    handler_[i] = nullptr;
  }

  epollfd_ = epoll_create(MAXFD);
  if(epollfd_ < 0){
    perror("epoll_create");
    exit(EXIT_FAILURE);
  }
}

EpollReactorImpl::~EpollReactorImpl(){
}

/*
 *--------------------------------------------------------------------------------------
 *       Class:  EpollReactorImpl
 *      Method:  EpollReactorImpl :: mRegisterHandler()
 * Description:  EventHandler objects register themselves to EpollReactorImpl through
 *         this method.
 *      Note:  This class only works well if the file descriptor assigned by kernel 
 *           each time a socket is created is lowest one available. It means that
 *           it only accepts socket with descriptor number < MAXFD. Ouch !!!
 *--------------------------------------------------------------------------------------
 */
void EpollReactorImpl::register_handler(EventHandler* eh, EventType et){
  Socket sockfd = eh->get_handle();
  if(sockfd >= MAXFD){
    //    pantheios::log_INFORMATIONAL("Too large file descriptor !!!");
    return;
  }
  
  struct epoll_event add_event;
  add_event.data.fd = sockfd;
  add_event.events = 0; //reset registered events

  if((et & READ_EVENT) == READ_EVENT)   
    add_event.events |= EPOLLIN;
  if((et & WRITE_EVENT) == WRITE_EVENT)
    add_event.events |= EPOLLOUT;
  
  if(epoll_ctl(epollfd_, EPOLL_CTL_ADD, sockfd, &add_event) < 0){
    perror("epoll_ctl ADD");
    return;
  }
  handler_[sockfd] = eh;
}

void EpollReactorImpl::register_handler(Socket h, EventHandler* eh, EventType et){
  //temporarily unavailable
  std::cout << "Method is not implemented!" << std::endl;
}

/*
 *--------------------------------------------------------------------------------------
 *       Class:  EpollReactorImpl
 *      Method:  EpollReactorImpl :: mRemoveHandler()
 * Description:  EventHandler objects deregister themselves to EpollReactorImpl through
 *         this method.
 *--------------------------------------------------------------------------------------
 */
void EpollReactorImpl::remove_handler(EventHandler* eh, EventType et){
  Socket sockfd = eh->get_handle();
  if(sockfd >= MAXFD){
    //    pantheios::log_INFORMATIONAL("Bad file descriptor !!!");
    return;
  }
  
  struct epoll_event rm_event;
  rm_event.data.fd = sockfd;
  if((et & READ_EVENT) == READ_EVENT)
    rm_event.events |= EPOLLIN;
  if((et & WRITE_EVENT) == WRITE_EVENT)
    rm_event.events |= EPOLLOUT;

  if(epoll_ctl(epollfd_, EPOLL_CTL_DEL, sockfd, &rm_event) < 0){
    perror("epoll_ctl DEL");
    return;
  }

  handler_[sockfd] = nullptr;
}

void EpollReactorImpl::remove_handler(Socket h, EventType et){
  //temporarily unavailable
  std::cout << "Method is not implemented!" << std::endl;
}

/*
 *--------------------------------------------------------------------------------------
 *       Class:  EpollReactorImpl
 *      Method:  EpollReactorImpl :: mHandleEvents()
 * Description:  Waiting for events using epoll_wait() function.
 *--------------------------------------------------------------------------------------
 */
void EpollReactorImpl::handle_events(TimeValue* time){
  int nevents;
  Socket temp;
  int timeout;

  if(time == nullptr) {
    timeout = -1;
  } else {
    timeout = (time->tv_sec)*1000 + (time->tv_usec)/1000;
  }

  nevents = epoll_wait(epollfd_, events_, MAXFD, timeout);
  if(nevents < 0){
    perror("epoll_wait");
    return;
  }

  for(int i = 0; i < nevents; i++){
    temp = events_[i].data.fd;
    if((events_[i].events & EPOLLIN) == EPOLLIN)
      handler_[temp]->handle_event(temp, READ_EVENT);

    //////////////////////////////////////////////////////////////////////////////////////////
    //NOTE: be careful in following case
    // - EventHandler register both EPOLLIN and EPOLLOUT event
    // - Client associated with that EventHandler close connection
    // - EPOLLIN & EPOLLOUT events return from epoll_wait()
    // - Upon processing EPOLLIN event, we free EventHandler (because connection is closed)
    // and set mHandler[i] to NULL.
    // - Afterward, EPOLLOUT event is processed, mHandleEvent() is call,
    // but mHandler[i] now = NULL, so this cause SEGMENTATION FAULT !!!!!!
    //
    // =>> One solution is: Process EPOLLOUT first, before EPOLLIN is processed.
    //////////////////////////////////////////////////////////////////////////////////////////
    if((events_[i].events & EPOLLOUT) == EPOLLOUT)
      handler_[temp]->handle_event(temp, WRITE_EVENT);
  }
}

#endif // HAS_EPOLL

/*
 * =====================================================================================
 *        Class:  KqueueReactorImpl
 *  Description:  This is implementation of Reactor with FreeBSD's kqueue facility.
 *       Note:  struct kevent {
 *            uintptr_t ident;  //it is often file descriptor
 *            short   filter; //flag indicates filter for above fd
 *            u_short   flags;  //flag indicates action with above fd or
 *                      //returned values from kevent()
 *                      
 *            u_int   fflags; //it is filter flags when registering or
 *                      //returned events retrieved by user
 *
 *            intptr_t  data; //data of filter
 *            void*   udata;  //user data
 *          };
 *
 *  Predefined filters: EVFILT_READ, EVFILT_WRITE, EVFILT_AIO, EVFILT_VNODE, 
 *            EVFILT_PROC, EVFILT_SIGNAL
 *       Input flags: EV_ADD, EV_DELETE, EV_ENABLE, EV_DISABLE, EV_CLEAR, EV_ONESHOT
 *      Output flags: EV_EOF, EV_ERROR
 *      Filter flags: (EVFILT_VNODE) NOTE_DELETE, NOTE_WRITE, NOTE_EXTEND, NOTE_ATTRIB,
 *              NOTE_LINK, NOTE_RENAME
 *              (EVFILT_PROC) NOTE_EXIT, NOTE_FORK, NOTE_EXEC, NOTE_TRACK,
 *              NOTE_CHILD, NOTE_TRACKERR
 *
 * =====================================================================================
 */
#ifdef HAS_KQUEUE

/*
 *--------------------------------------------------------------------------------------
 *       Class:  KqueueReactorImpl
 *      Method:  KqueueReactorImpl :: KqueueReactorImpl()
 * Description:  Constructor creates new kqueue instance.
 *--------------------------------------------------------------------------------------
 */
KqueueReactorImpl::KqueueReactorImpl(){
  events_no_ = 0;
  kqueue_ = kqueue();
  if(kqueue_ == -1){
    perror("kqueue() error");
    exit(EXIT_FAILURE);
  }
}

KqueueReactorImpl::~KqueueReactorImpl(){
}

/*
 *--------------------------------------------------------------------------------------
 *       Class:  KqueueReactorImpl
 *      Method:  KqueueReactorImpl :: mRegisterHandler()
 * Description:  Register new socket descriptor to kqueue. Called when new socket is
 *         created.
 *--------------------------------------------------------------------------------------
 */
void KqueueReactorImpl::register_handler(EventHandler* eh, EventType et){
  Socket temp = eh->get_handle();
  struct kevent event;
  u_short flags = 0;

  if((et & READ_EVENT) == READ_EVENT)
    flags |= EVFILT_READ;
  if((et & WRITE_EVENT) == WRITE_EVENT)
    flags |= EVFILT_WRITE;

  //we set pointer to EventHandler to event, so we have handler associated with
  //that socket when receive return from kevent() function.
  //Prototype:
  //  EV_SET(&kev, ident, filter, flags, fflags, data, udata);
  ///////////////
  
  EV_SET(&event, temp, flags, EV_ADD | EV_ENABLE, 0, 0, eh);

  if(kevent(kqueue_, &event, 1, NULL, 0, NULL) == -1){
    perror("kevent() error");
    return;
  }

  events_no_++;
}

void KqueueReactorImpl::register_handler(Socket h, EventHandler* eh, EventType et){
  std::cout << "Method is not implemented!" << std::endl;
}

/*
 *--------------------------------------------------------------------------------------
 *       Class:  KqueueReactorImpl
 *      Method:  KqueueReactorImpl :: mRemoveHandler()
 * Description:  Remove existing socket in kqueue. 
 *--------------------------------------------------------------------------------------
 */
void KqueueReactorImpl::remove_handler(EventHandler* eh, EventType et){
  Socket temp = eh->get_handle();
  struct kevent event;
  u_short flags = 0;

  if((et & READ_EVENT) == READ_EVENT)
    flags |= EVFILT_READ;
  if((et & WRITE_EVENT) == WRITE_EVENT)
    flags |= EVFILT_WRITE;

  EV_SET(&event, temp, flags, EV_DELETE, 0, 0, 0);

  if(kevent(kqueue_, &event, 1, NULL, 0, NULL) == -1){
    perror("kevent() error");
    return;
  }

  events_no_--;
}

void KqueueReactorImpl::remove_handler(Socket h, EventType et){
  std::cout << "Method is not implemented!" << std::endl;
}

/*
 *--------------------------------------------------------------------------------------
 *       Class:  KqueueReactorImpl
 *      Method:  KqueueReactorImpl :: mHandleEvents()
 * Description:  Waiting for events come to sockets in kqueue
 *--------------------------------------------------------------------------------------
 */
void KqueueReactorImpl::handle_events(TimeValue* time){
  int nevents;
  struct kevent ev[events_no_];
  struct timespec* tout;

  if(time == nullptr)
    tout = nullptr;
  else{
    tout = new timespec;
    tout->tv_sec = time->tv_sec;
    tout->tv_nsec = time->tv_usec*1000;
  }

  nevents = kevent(kqueue_, NULL, 0, ev, events_no_, tout);
  if(nevents < 0){
    if(tout != nullptr)
      delete tout;
    perror("kevent() error");
    return;
  }
  
  if(tout != nullptr)
    delete tout;

  for(int i = 0; i < nevents; i++){
    if(ev[i].filter == EVFILT_READ)
      (EventHandler*)ev[i].udata->handle_event(ev[i].ident, READ_EVENT);
    if(ev[i].filter == EVFILT_WRITE)
      (EventHandler*)ev[i].udata->handle_event(ev[i].ident, WRITE_EVENT);
  }
}

#endif // HAS_KQUEUE
