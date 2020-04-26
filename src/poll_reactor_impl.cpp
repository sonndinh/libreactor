#include "reactor_impl.h"

PollReactorImpl::PollReactorImpl() {
  for (int i = 0; i < MAXFD; i++) {
    client_[i].fd = -1;
    client_[i].events = 0;
    client_[i].revents = 0;
    handler_[i] = nullptr;
  }
  maxi_ = 0;
}

PollReactorImpl::~PollReactorImpl() {
}

/**
 * @brief Waiting for events using poll() function, then dispatch them
 * to the corresponding event handlers.
 */
void PollReactorImpl::handle_events(TimeValue* time) {
  int nready, timeout, i;
  if (time == nullptr)
    timeout = -1;
  else
    timeout = (time->tv_sec)*1000 + (time->tv_usec)/1000;

  nready = poll(client_, maxi_+1, timeout);
  if (nready < 0) {
    perror("poll() error");
    return;
  }

  for (i = 0; i <= maxi_; i++) {
    if (client_[i].fd < 0)
      continue;
    
    if ((client_[i].revents & POLLRDNORM)) {
      handler_[i]->handle_event(client_[i].fd, READ_EVENT);
      if (--nready <= 0)
        break;
    }
    
    if ((client_[i].revents & POLLWRNORM)) {
      handler_[i]->handle_event(client_[i].fd, WRITE_EVENT);
      if (--nready <= 0)
        break;
    }
  }
}

void PollReactorImpl::register_handler(EventHandler* eh, EventType et) {
  int i;
  for (i = 0; i < MAXFD; i++) {
    // First element in array has fd=-1 will be fill
    // with new handler. Then we break out of for loop
    if (client_[i].fd == -1) {
      client_[i].fd = eh->get_handle();
      client_[i].events = 0; //reset events field

      if ((et & READ_EVENT) == READ_EVENT)
        client_[i].events |= POLLRDNORM;
      if ((et & WRITE_EVENT) == WRITE_EVENT)
        client_[i].events |= POLLWRNORM;

      handler_[i] = eh;
      break;
    }
  }

  if (i == MAXFD) {
    exit(EXIT_FAILURE);
  }

  if (i > maxi_)
    maxi_ = i;  
}

void PollReactorImpl::register_handler(Socket h, EventHandler* eh, EventType et) {
  // Temporary unavailable
  std::cout << "Method is not implemented!" << std::endl;
}

void PollReactorImpl::remove_handler(EventHandler* eh, EventType et) {
  for (int i = 0; i <= maxi_; i++) {
    if (client_[i].fd == eh->get_handle()) {
      client_[i].fd = -1;
      client_[i].events = 0;
      handler_[i] = nullptr;
      break;
    }
  }
}

void PollReactorImpl::remove_handler(Socket h, EventType et) {
  for (int i = 0; i < maxi_; i++) {
    if (client_[i].fd == h) {
      client_[i].fd = -1;
      handler_[i] = nullptr;
      break;
    }
  }
}
