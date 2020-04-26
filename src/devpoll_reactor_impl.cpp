#include "reactor_impl.h"

#ifdef HAS_DEV_POLL

DevPollReactorImpl::DevPollReactorImpl() {
  devpollfd_ = open("/dev/poll", O_RDWR);
  if (devpollfd_ < 0) {
    exit(EXIT_FAILURE);
  }

  // Init input array of struct pollfd
  for (int i = 0; i < MAXFD; i++) {
    buf_[i].fd = -1;
    buf_[i].events = 0;
    buf_[i].revents = 0;
    handler_[i] = nullptr;
  }

  // Init output array of pollfd has event on
  output_ = (struct pollfd*) malloc(sizeof(struct pollfd) * MAXFD);
}

DevPollReactorImpl::~DevPollReactorImpl() {
  close(devpollfd_);
  if (output_ != nullptr) {
    free(output_);
  }
}

void DevPollReactorImpl::handle_events(TimeValue* time) {
  int nready, timeout, i;
  if (time == nullptr) {
    timeout = -1;
  } else {
    timeout = (time->tv_sec)*1000 + (time->tv_usec)/1000;
  }

  struct dvpoll dopoll;
  dopoll.dp_timeout = timeout;
  dopoll.dp_nfds = MAXFD;
  dopoll.dp_fds = output_;

  // Waiting for events
  nready = ioctl(devpollfd_, DP_POLL, &dopoll);

  if (nready < 0) {
    perror("/dev/poll ioctl DP_POLL failed");
    return;
  }

  for (i = 0; i < nready; i++) {
    int temp = (output_ + i)->fd;

    if ((output_ + i)->revents & POLLRDNORM) {
      handler_[temp]->handle_event(temp, READ_EVENT);
    }
    if ((output_ + i)->revents & POLLWRNORM) {
      handler_[temp]->handle_event(temp, WRITE_EVENT);
    }
  }
}

void DevPollReactorImpl::register_handler(EventHandler* eh, EventType et) {
  int temp = eh->get_handle();

  for (int i = 0; i < MAXFD; i++) {
    if (buf_[i].fd < 0) {
      buf_[i].fd = eh->get_handle();
      buf_[i].events = 0; //reset events field

      if ((et & READ_EVENT) == READ_EVENT) {
        buf_[i].events |= POLLRDNORM;
      }
      if ((et & WRITE_EVENT) == WRITE_EVENT) {
        buf_[i].events |= POLLWRNORM;
      }

      // Handler of a particular descriptor is saved to index has
      // the same value with the descriptor
      handler_[temp] = eh;
      break;
    }
  }

  if (i == MAXFD) {
    exit(EXIT_FAILURE);
  }

  // Close old mDevpollfd file and open new file rely on new set of pollfds
  close(devpollfd_);
  devpollfd_ = open("/dev/poll", O_RDWR);
  if (devpollfd_ < 0) {
    perror("Cannot open /dev/poll !!!");
    exit(EXIT_FAILURE);
  }

  if (write(devpollfd_, buf_, (sizeof(struct pollfd) * MAXFD)) != 
     (sizeof(struct pollfd) * MAXFD) ) {
    perror("Failed to write all pollfds");
    close(devpollfd_);
    if (output_ != nullptr) {
      free(output_);
      output_ = nullptr;
    }
    exit(EXIT_FAILURE);
  }
}

void DevPollReactorImpl::register_handler(Socket h, EventHandler* eh, EventType et) {
  // Temporarily unavailable
  std::cout << "Method is not implemented!" << std::endl;
}

void DevPollReactorImpl::remove_handler(EventHandler* eh, EventType et) {
  int temp = eh->get_handle();
  remove_handler(temp, et);
}

void DevPollReactorImpl::remove_handler(Socket h, EventType et) {

  // Find the pollfd which has the same file descriptor
  for (int i = 0; i < MAXFD; i++) {
    if (buf_[i].fd == h) {
      buf_[i].fd = -1;
      buf_[i].events = 0;
      handler_[h] = nullptr;
      break;
    }
  }

  // Rewrite the /dev/poll
  close(devpollfd_);
  devpollfd_ = open("/dev/poll", O_RDWR);
  if (devpollfd_ < 0) {
    perror("Cannot open /dev/poll !!!");
    exit(EXIT_FAILURE);
  }

  if (write(devpollfd_, buf_, (sizeof(struct pollfd) * MAXFD)) != 
     (sizeof(struct pollfd) * MAXFD) ) {
    perror("Failed to write all pollfds");
    close(devpollfd_);
    if (output_ != null) {
      free(output_);
      output_ = nullptr;
    }
    exit(EXIT_FAILURE);
  }
}

#endif // HAS_DEV_POLL
