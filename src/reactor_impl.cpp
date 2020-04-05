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
 *       Class:  SelectReactorImpl
 *      Method:  SelectReactorImpl :: SelectReactorImpl()
 * Description:  Constructor of SelectReactorImpl class
 *--------------------------------------------------------------------------------------
 */
SelectReactorImpl::SelectReactorImpl(){
  FD_ZERO(&rdset_);
  FD_ZERO(&wrset_);
  FD_ZERO(&exset_);
  max_handle_ = 0;
}

SelectReactorImpl::~SelectReactorImpl(){
}

/*
 *--------------------------------------------------------------------------------------
 *       Class:  SelectReactorImpl
 *      Method:  SelectReactorImpl :: mHandleEvents()
 * Description:  Waiting for events using select() function
 *--------------------------------------------------------------------------------------
 */
void SelectReactorImpl::handle_events(TimeValue* timeout){
  fd_set readset, writeset, exceptset;
  readset = rdset_;
  writeset = wrset_;
  exceptset = exset_;

  int result = select(max_handle_+1, &readset, &writeset, &exceptset, timeout);
  if(result < 0){
    //exit(EXIT_FAILURE);
    perror("select() error");
    return;
  }
  //pantheios::log_INFORMATIONAL("The number of ready events: ", pantheios::integer(result));

  for(Socket h = 0; h <= max_handle_; h++){
    //We should check for incoming events in each SOCKET
    if(FD_ISSET(h, &readset)){
      (table_.table_[h].event_handler)->handle_event(h, READ_EVENT);
      if(--result <= 0)
        break;
      continue;
    }
    if(FD_ISSET(h, &writeset)){
      (table_.table_[h].event_handler)->handle_event(h, WRITE_EVENT);
      if(--result <= 0)
        break;
      continue;
    }
    if(FD_ISSET(h, &exceptset)){
      (table_.table_[h].event_handler)->handle_event(h, EXCEPT_EVENT);
      if(--result <= 0)
        break;
      continue;
    }
  }
}

/*
void SelectReactorImpl::mHandleEvents(Time_Value* timeout){
  SOCKET max_handle;
  fd_set readset, writeset, exceptset;
  FD_ZERO(&readset);
  FD_ZERO(&writeset);
  FD_ZERO(&exceptset);
  mTable.mConvertToFdSets(readset, writeset, exceptset, max_handle);
  
  pantheios::log_INFORMATIONAL("Max_handle: ", pantheios::integer(max_handle), ". Demultiplex using select()");
  int result = select(max_handle+1, &readset, &writeset, &exceptset, timeout);
  pantheios::log_INFORMATIONAL("Result after select(): ", pantheios::integer(result));

  if(result < 0)  throw //handle error here

  for(SOCKET h=0; h<=max_handle; h++){
    //We should check for incoming events in each SOCKET
    if(FD_ISSET(h, &readset)){
      (mTable.mTable[h].mEventHandler)->mHandleEvent(h, READ_EVENT);
      continue;
    }
    if(FD_ISSET(h, &writeset)){
      (mTable.mTable[h].mEventHandler)->mHandleEvent(h, WRITE_EVENT);
      continue;
    }
    if(FD_ISSET(h, &exceptset)){
      (mTable.mTable[h].mEventHandler)->mHandleEvent(h, EXCEPT_EVENT);
      continue;
    }
  }
}
*/

/*
 *--------------------------------------------------------------------------------------
 *       Class:  SelectReactorImpl
 *      Method:  SelectReactorImpl :: mRegisterHandler()
 * Description:  Each time StreamHandler or ConnectionAcceptor or other EventHandler
 *         object is created, it registers to this object to receive favor events
 *--------------------------------------------------------------------------------------
 */
void SelectReactorImpl::register_handler(EventHandler* eh, EventType et){
  //Get SOCKET associated with this EventHandler object
  Socket temp = eh->get_handle();
  table_.table_[temp].event_handler = eh;
  table_.table_[temp].event_type = et;
  
  //set maximum handle value
  if(temp > max_handle_)
    max_handle_ = temp;

  //set appropriate bits to mRdSet, mWrSet and mExSet
  if((et & READ_EVENT) == READ_EVENT)
    FD_SET(temp, &rdset_);
  if((et & WRITE_EVENT) == WRITE_EVENT)
    FD_SET(temp, &wrset_);
  if((et & EXCEPT_EVENT) == EXCEPT_EVENT)
    FD_SET(temp, &exset_);
}

void SelectReactorImpl::register_handler(Socket h, EventHandler* eh, EventType et){
  //This is temporarily unavailable
}

/*
 *--------------------------------------------------------------------------------------
 *       Class:  SelectReactorImpl
 *      Method:  SelectReactorImpl :: mRemoveHandler()
 * Description:  When StreamHandler or ConnectionAcceptor or other EventHandler object
 *         is destructed, it deregisters to Reactor so Reactor doesn't dispatch
 *         events to it anymore.
 *--------------------------------------------------------------------------------------
 */
void SelectReactorImpl::remove_handler(EventHandler* eh, EventType et){
  Socket temp = eh->get_handle();
  table_.table_[temp].event_handler = nullptr;
  table_.table_[temp].event_type = 0;

  //clear appropriate bits to mRdSet, mWrSet, and mExSet
  if((et & READ_EVENT) == READ_EVENT)
    FD_CLR(temp, &rdset_);
  if((et & WRITE_EVENT) == WRITE_EVENT)
    FD_CLR(temp, &wrset_);
  if((et & EXCEPT_EVENT) == EXCEPT_EVENT)
    FD_CLR(temp, &exset_);
}

void SelectReactorImpl::remove_handler(Socket h, EventType et){
  table_.table_[h].event_handler = nullptr;
  table_.table_[h].event_type = 0;

  //clear appropriate bits to mRdSet, mWrSet, and mExSet
  if((et & READ_EVENT) == READ_EVENT)
    FD_CLR(h, &rdset_);
  if((et & WRITE_EVENT) == WRITE_EVENT)
    FD_CLR(h, &wrset_);
  if((et & EXCEPT_EVENT) == EXCEPT_EVENT)
    FD_CLR(h, &exset_);
}


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
 *--------------------------------------------------------------------------------------
 *       Class:  PollReactorImpl
 *      Method:  PollReactorImpl :: PollReactorImpl()
 * Description:  Constructor which inits input array for poll()
 *--------------------------------------------------------------------------------------
 */
PollReactorImpl::PollReactorImpl(){
  for(int i = 0; i < MAXFD; i++){
    client_[i].fd = -1;
    client_[i].events = 0;
    client_[i].revents = 0;
    handler_[i] = nullptr;
  }
  maxi_ = 0;
}

PollReactorImpl::~PollReactorImpl(){
}

/*
 *--------------------------------------------------------------------------------------
 *       Class:  PollReactorImpl
 *      Method:  PollReactorImpl :: mHandleEvents()
 * Description:  Waiting for events using poll() function, then dispatch these events
 *         to corresponding EventHandler object.
 *--------------------------------------------------------------------------------------
 */
void PollReactorImpl::handle_events(TimeValue* time){
  int nready, timeout, i;
  if(time == nullptr)
    timeout = -1;
  else
    timeout = (time->tv_sec)*1000 + (time->tv_usec)/1000;

  nready = poll(client_, maxi_+1, timeout);
  if(nready < 0){
    perror("poll() error");
    return;
  }

  for(i = 0; i <= maxi_; i++){
    if(client_[i].fd < 0)
      continue;
    
    if((client_[i].revents & POLLRDNORM)){
      //      pantheios::log_INFORMATIONAL("Read event at mClient[", pantheios::integer(i), "]");
      handler_[i]->handle_event(client_[i].fd, READ_EVENT);
      if(--nready <= 0)
        break;
    }
    if((client_[i].revents & POLLWRNORM)){
      //      pantheios::log_INFORMATIONAL("Write event at mClient[", pantheios::integer(i), "]");
      handler_[i]->handle_event(client_[i].fd, WRITE_EVENT);
      if(--nready <= 0)
        break;
    }   
  }
}

/*
 *--------------------------------------------------------------------------------------
 *       Class:  PollReactorImpl
 *      Method:  PollReactorImpl :: mRegisterHandler()
 * Description:  Used by StreamHandler, ConnectionAcceptor or other further EventHandler
 *         object to register its handler.
 *--------------------------------------------------------------------------------------
 */
void PollReactorImpl::register_handler(EventHandler* eh, EventType et){
  int i;
  for(i = 0; i < MAXFD; i++)
    //First element in array has fd=-1 will be fill
    //with new handler. Then we break out of for loop
    if(client_[i].fd == -1){
      client_[i].fd = eh->get_handle();
      client_[i].events = 0; //reset events field

      if((et & READ_EVENT) == READ_EVENT)
        client_[i].events |= POLLRDNORM;
      if((et & WRITE_EVENT) == WRITE_EVENT)
        client_[i].events |= POLLWRNORM;

      handler_[i] = eh;
      break;
    }

  if(i == MAXFD){
    //    pantheios::log_INFORMATIONAL("Too many client !!!");
    exit(EXIT_FAILURE);
  }

  //  pantheios::log_INFORMATIONAL("mMaxi: ", pantheios::integer(mMaxi), ". i: ", pantheios::integer(i));
  if(i > maxi_)
    maxi_ = i;  
}

void PollReactorImpl::register_handler(Socket h, EventHandler* eh, EventType et){
  //Temporary unavailable
  std::cout << "Method is not implemented!" << std::endl;
}

/*
 *--------------------------------------------------------------------------------------
 *       Class:  PollReactorImpl
 *      Method:  PollReactorImpl :: mRemoveHandler()
 * Description:  Called by ConnectionAcceptor, StreamHandler or other EventHandler when
 *         it is destructed, so events are no longer dispatched to them.
 *--------------------------------------------------------------------------------------
 */
void PollReactorImpl::remove_handler(EventHandler* eh, EventType et){
  for(int i = 0; i <= maxi_; i++){
    if(client_[i].fd == eh->get_handle()){
      client_[i].fd = -1;
      client_[i].events = 0;
      handler_[i] = nullptr;
      break;
    }
  }
}

void PollReactorImpl::remove_handler(Socket h, EventType et){
  for(int i = 0; i < maxi_; i++){
    if(client_[i].fd == h){
      client_[i].fd = -1;
      handler_[i] = nullptr;
      break;
    }
  }
}


/*
 * =====================================================================================
 *        Class:  DevPollReactorImpl
 *  Description:  
 * =====================================================================================
 */
#ifdef HAS_DEV_POLL

DevPollReactorImpl::DevPollReactorImpl(){ 
  devpollfd_ = open("/dev/poll", O_RDWR);
  if(devpollfd_ < 0) {
    //    pantheios::log_INFORMATIONAL("Cannot open /dev/poll !!!");
    exit(EXIT_FAILURE);
  }

  //init input array of struct pollfd
  for(int i = 0; i < MAXFD; i++){
    buf_[i].fd = -1;
    buf_[i].events = 0;
    buf_[i].revents = 0;
    handler_[i] = nullptr;
  }

  //init output array of pollfd has event on
  output_ = (struct pollfd*) malloc(sizeof(struct pollfd) * MAXFD);
}

DevPollReactorImpl::~DevPollReactorImpl(){
  close(devpollfd_);
  if(output_ != nullptr) {
    free(output_);
  }
}

void DevPollReactorImpl::handle_events(TimeValue* time){
  int nready, timeout, i;
  if(time == nullptr) {
    timeout = -1;
  } else {
    timeout = (time->tv_sec)*1000 + (time->tv_usec)/1000;
  }

  struct dvpoll dopoll;
  dopoll.dp_timeout = timeout;
  dopoll.dp_nfds = MAXFD;
  dopoll.dp_fds = output_;

  //waiting for events
  nready = ioctl(devpollfd_, DP_POLL, &dopoll);

  if(nready < 0) {
    perror("/dev/poll ioctl DP_POLL failed");
    return;
  }

  for(i = 0; i < nready; i++) {
    int temp = (output_ + i)->fd;

    if((output_ + i)->revents & POLLRDNORM) {
      handler_[temp]->handle_event(temp, READ_EVENT);
    }
    if((output_ + i)->revents & POLLWRNORM) {
      handler_[temp]->handle_event(temp, WRITE_EVENT);
    }
  }
}

void DevPollReactorImpl::register_handler(EventHandler* eh, EventType et){
  int temp = eh->get_handle();

  for(int i = 0; i < MAXFD; i++) {
    if(buf_[i].fd < 0) {
      buf_[i].fd = eh->get_handle();
      buf_[i].events = 0; //reset events field

      if((et & READ_EVENT) == READ_EVENT) {
        buf_[i].events |= POLLRDNORM;
      }
      if((et & WRITE_EVENT) == WRITE_EVENT) {
        buf_[i].events |= POLLWRNORM;
      }

      //handler of a particular descriptor is saved to index has the same value with the descriptor
      handler_[temp] = eh;
      break;
    }
  }

  if(i == MAXFD) {
    //    pantheios::log_INFORMATIONAL("Too many client !");
    exit(EXIT_FAILURE);
  }

  //close old mDevpollfd file and open new file rely on new set of pollfds
  close(devpollfd_);
  devpollfd_ = open("/dev/poll", O_RDWR);
  if(devpollfd_ < 0) {
    perror("Cannot open /dev/poll !!!");
    exit(EXIT_FAILURE);
  }

  if(write(devpollfd_, buf_, (sizeof(struct pollfd) * MAXFD)) != 
     (sizeof(struct pollfd) * MAXFD) ) {
    perror("Failed to write all pollfds");
    close(devpollfd_);
    if(output_ != nullptr) {
      free(output_);
      output_ = nullptr;
    }
    exit(EXIT_FAILURE);
  }
}

void DevPollReactorImpl::register_handler(Socket h, EventHandler* eh, EventType et){
  //temporarily unavailable
  std::cout << "Method is not implemented!" << std::endl;
}

void DevPollReactorImpl::remove_handler(EventHandler* eh, EventType et){
  int temp = eh->get_handle();
  remove_handler(temp, et);
}

void DevPollReactorImpl::remove_handler(Socket h, EventType et){

  //find the pollfd which has the same file descriptor
  for(int i = 0; i < MAXFD; i++) {
    if(buf_[i].fd == h) {
      buf_[i].fd = -1;
      buf_[i].events = 0;
      handler_[h] = nullptr;
      break;
    }
  }

  //rewrite the /dev/poll
  close(devpollfd_);
  devpollfd_ = open("/dev/poll", O_RDWR);
  if(devpollfd_ < 0) {
    perror("Cannot open /dev/poll !!!");
    exit(EXIT_FAILURE);
  }

  if(write(devpollfd_, buf_, (sizeof(struct pollfd) * MAXFD)) != 
     (sizeof(struct pollfd) * MAXFD) ) {
    perror("Failed to write all pollfds");
    close(devpollfd_);
    if(output_ != null) {
      free(output_);
      output_ = nullptr;
    }
    exit(EXIT_FAILURE);
  }
}
#endif // HAS_DEV_POLL


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
