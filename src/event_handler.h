#ifndef EVENT_HANDLER_H_
#define EVENT_HANDLER_H_

#include "common.h"

/**
 * @class EventHandler
 *
 * @brief Interface for handling events dispatched by the reactor.
 */
class EventHandler {
public:
  // NOTE: may be we don't need "handle" parameter in mHandleEvent()
  // because when mHandleEvent() called, it can get associated socket
  // descriptor through SOCK_Acceptor or SOCK_Stream or SOCK_Datagram
  virtual void handle_event(Socket handle, EventType et) = 0;

  virtual Socket get_handle() const = 0;
};


#endif // EVENT_HANDLER_H_
