#ifndef CONNECTION_ACCEPTOR_H_
#define CONNECTION_ACCEPTOR_H_

#include "common.h"
#include "socket_wf.h"
#include "reactor.h"
#include "event_handler.h"

/**
 * @class ConnectionAcceptor
 *
 * @brief Handle TCP connection requests from clients.
 */
class ConnectionAcceptor : public EventHandler {
public:
  ConnectionAcceptor(const InetAddr &addr, Reactor* reactor);
  ~ConnectionAcceptor();

  virtual void handle_event(Socket handle, EventType et);
  virtual Socket get_handle() const;

private:
  //Socket factory that accepts client connections
  SockAcceptor* sock_acceptor_;

  //Cached Reactor
  Reactor* reactor_;
};


#endif // CONNECTION_ACCEPTOR_H_
