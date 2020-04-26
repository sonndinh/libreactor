/**
 * @brief This is implementation of Reactor with FreeBSD's kqueue facility.
 * Note:  struct kevent {
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
 */
#ifdef HAS_KQUEUE
#include "reactor_impl.h"

KqueueReactorImpl::KqueueReactorImpl() {
  events_no_ = 0;
  kqueue_ = kqueue();
  if (kqueue_ == -1) {
    perror("kqueue() error");
    exit(EXIT_FAILURE);
  }
}

KqueueReactorImpl::~KqueueReactorImpl() {
}

void KqueueReactorImpl::register_handler(EventHandler* eh, EventType et) {
  Socket temp = eh->get_handle();
  struct kevent event;
  u_short flags = 0;

  if ((et & READ_EVENT) == READ_EVENT)
    flags |= EVFILT_READ;
  if ((et & WRITE_EVENT) == WRITE_EVENT)
    flags |= EVFILT_WRITE;

  //we set pointer to EventHandler to event, so we have handler associated with
  //that socket when receive return from kevent() function.
  //Prototype:
  //  EV_SET(&kev, ident, filter, flags, fflags, data, udata);
  ///////////////
  
  EV_SET(&event, temp, flags, EV_ADD | EV_ENABLE, 0, 0, eh);

  if (kevent(kqueue_, &event, 1, NULL, 0, NULL) == -1) {
    perror("kevent() error");
    return;
  }

  events_no_++;
}

void KqueueReactorImpl::register_handler(Socket h, EventHandler* eh, EventType et) {
  std::cout << "Method is not implemented!" << std::endl;
}

/**
 * @brief Remove existing socket in kqueue.
 *--------------------------------------------------------------------------------------
 */
void KqueueReactorImpl::remove_handler(EventHandler* eh, EventType et) {
  Socket temp = eh->get_handle();
  struct kevent event;
  u_short flags = 0;

  if ((et & READ_EVENT) == READ_EVENT)
    flags |= EVFILT_READ;
  if ((et & WRITE_EVENT) == WRITE_EVENT)
    flags |= EVFILT_WRITE;

  EV_SET(&event, temp, flags, EV_DELETE, 0, 0, 0);

  if (kevent(kqueue_, &event, 1, NULL, 0, NULL) == -1) {
    perror("kevent() error");
    return;
  }

  events_no_--;
}

void KqueueReactorImpl::remove_handler(Socket h, EventType et) {
  std::cout << "Method is not implemented!" << std::endl;
}

/**
 * @brief Waiting for events come to sockets in kqueue.
 */
void KqueueReactorImpl::handle_events(TimeValue* time) {
  int nevents;
  struct kevent ev[events_no_];
  struct timespec* tout;

  if (time == nullptr) {
    tout = nullptr;
  } else {
    tout = new timespec;
    tout->tv_sec = time->tv_sec;
    tout->tv_nsec = time->tv_usec*1000;
  }

  nevents = kevent(kqueue_, NULL, 0, ev, events_no_, tout);
  if (nevents < 0) {
    if (tout != nullptr)
      delete tout;
    perror("kevent() error");
    return;
  }
  
  if (tout != nullptr)
    delete tout;

  for (int i = 0; i < nevents; i++) {
    if (ev[i].filter == EVFILT_READ)
      (EventHandler*)ev[i].udata->handle_event(ev[i].ident, READ_EVENT);
    if (ev[i].filter == EVFILT_WRITE)
      (EventHandler*)ev[i].udata->handle_event(ev[i].ident, WRITE_EVENT);
  }
}

#endif // HAS_KQUEUE
