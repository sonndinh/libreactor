#ifdef HAS_EPOLL
#include "reactor_impl.h"

EpollReactorImpl::EpollReactorImpl() {
  for (int i = 0; i < MAXFD; i++) {
    handler_[i] = nullptr;
  }

  epollfd_ = epoll_create(MAXFD);
  if (epollfd_ < 0) {
    perror("epoll_create");
    exit(EXIT_FAILURE);
  }
}

EpollReactorImpl::~EpollReactorImpl() {
}

/**
 * @brief Event handlers register themselves using this method.
 * Note: This class only works well if the file descriptor assigned by kernel 
 * each time a socket is created is lowest one available. It means that
 * it only accepts socket with descriptor number < MAXFD!
 */
void EpollReactorImpl::register_handler(EventHandler* eh, EventType et) {
  Socket sockfd = eh->get_handle();
  if (sockfd >= MAXFD) {
    return;
  }
  
  struct epoll_event add_event;
  add_event.data.fd = sockfd;
  add_event.events = 0; // Reset registered events

  if ((et & READ_EVENT) == READ_EVENT)   
    add_event.events |= EPOLLIN;
  if ((et & WRITE_EVENT) == WRITE_EVENT)
    add_event.events |= EPOLLOUT;
  
  if (epoll_ctl(epollfd_, EPOLL_CTL_ADD, sockfd, &add_event) < 0) {
    perror("epoll_ctl ADD");
    return;
  }
  handler_[sockfd] = eh;
}

void EpollReactorImpl::register_handler(Socket h, EventHandler* eh, EventType et) {
  // Temporarily unavailable
  std::cout << "Method is not implemented!" << std::endl;
}

void EpollReactorImpl::remove_handler(EventHandler* eh, EventType et) {
  Socket sockfd = eh->get_handle();
  if (sockfd >= MAXFD) {
    return;
  }
  
  struct epoll_event rm_event;
  rm_event.data.fd = sockfd;
  if ((et & READ_EVENT) == READ_EVENT)
    rm_event.events |= EPOLLIN;
  if ((et & WRITE_EVENT) == WRITE_EVENT)
    rm_event.events |= EPOLLOUT;

  if (epoll_ctl(epollfd_, EPOLL_CTL_DEL, sockfd, &rm_event) < 0) {
    perror("epoll_ctl DEL");
    return;
  }

  handler_[sockfd] = nullptr;
}

void EpollReactorImpl::remove_handler(Socket h, EventType et) {
  // Temporarily unavailable
  std::cout << "Method is not implemented!" << std::endl;
}

/**
 * @brief Waiting for events using epoll_wait.
 */
void EpollReactorImpl::handle_events(TimeValue* time) {
  int nevents;
  Socket temp;
  int timeout;

  if (time == nullptr) {
    timeout = -1;
  } else {
    timeout = (time->tv_sec)*1000 + (time->tv_usec)/1000;
  }

  nevents = epoll_wait(epollfd_, events_, MAXFD, timeout);
  if (nevents < 0) {
    perror("epoll_wait");
    return;
  }

  for (int i = 0; i < nevents; i++) {
    temp = events_[i].data.fd;
    if ((events_[i].events & EPOLLIN) == EPOLLIN)
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
    if ((events_[i].events & EPOLLOUT) == EPOLLOUT)
      handler_[temp]->handle_event(temp, WRITE_EVENT);
  }
}

#endif // HAS_EPOLL
