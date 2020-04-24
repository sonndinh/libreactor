#ifndef UDP_HANDLER_H_
#define UDP_HANDLER_H_

#include "common.h"

class InetAddr;
class Reactor;
class SockDatagram;


/**
 * @class UdpHandler
 *
 * @brief Handle data from UDP clients.
 */
class UdpHandler : public EventHandler {
public: 
  UdpHandler(const InetAddr& addr, Reactor* reactor);
  ~UdpHandler();
  
  virtual void handle_event(Socket sockfd, EventType et);
  virtual Socket get_handle() const;

protected:
  virtual void handle_read(Socket sockfd);
  virtual void handle_write(Socket sockfd);
  virtual void handle_except(Socket sockfd);

private:
  SockDatagram* sock_dgram_;
  
  Reactor* reactor_;
};

#endif // UDP_HANDLER_H_
