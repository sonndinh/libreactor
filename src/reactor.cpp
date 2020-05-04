#include "reactor.h"
#include "reactor_impl.h"

Reactor* Reactor::reactor_ = nullptr;

ReactorImpl* Reactor::reactor_impl_ = nullptr;

/**
 * @brief Delegate to a concrete implementation of Reactor.
 */
void Reactor::register_handler(EventHandler* eh, EventType et) {
  reactor_impl_->register_handler(eh, et);
}

void Reactor::register_handler(Socket h, EventHandler* eh, EventType et) {
  reactor_impl_->register_handler(h, eh, et);
}

void Reactor::remove_handler(EventHandler* eh, EventType et) {
  reactor_impl_->remove_handler(eh, et);
}

void Reactor::remove_handler(Socket h, EventType et) {
  reactor_impl_->remove_handler(h, et);
}

ReactorImpl* Reactor::get_reactor_impl() {
  return reactor_impl_;
}

/**
 * @brief Call to demultiplexer to wait for events.
 */
void Reactor::handle_events(TimeValue* timeout) {
  reactor_impl_->handle_events(timeout);
}


/**
 * @brief Get the sole instance of this class.
 * In the first call, need to explicitly indicate which 
 * demultiplexer will be used if it is not SELECT_DEMUX.
 */
Reactor* Reactor::instance(DemuxType demux) {
  if (reactor_ == nullptr) {
    switch (demux) {
    case SELECT_DEMUX:
      reactor_impl_ = new SelectReactorImpl();
      break;
    case POLL_DEMUX:
      reactor_impl_ = new PollReactorImpl();
      break;

#if defined (HAS_DEV_POLL)
    case DEVPOLL_DEMUX:
      reactor_impl_ = new DevPollReactorImpl();
      break;
#endif // HAS_DEV_POLL
      
#if defined (HAS_EPOLL)
    case EPOLL_DEMUX:
      reactor_impl_ = new EpollReactorImpl();
      break;
#endif // HAS_EPOLL
      
#if defined (HAS_KQUEUE)
    case KQUEUE_DEMUX:
      reactor_impl_ = new KqueueReactorImpl();
      break;
#endif // HAS_KQUEUE
      
    default:
      break;
    }

    // Use "lazy" initialization for Reactor.
    reactor_ = new Reactor();
  }

  return reactor_;
}

Reactor::Reactor() {
  tcp_read_handler_ = nullptr;
  tcp_event_handler_ = nullptr;
  udp_read_handler_ = nullptr;
  udp_event_handler_ = nullptr;
}

Reactor::~Reactor() {
  delete reactor_impl_;
}
