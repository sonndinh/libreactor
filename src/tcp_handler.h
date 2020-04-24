#ifndef TCP_HANDLER_H_
#define TCP_HANDLER_H_

#include "common.h"

class Reactor;
class SockStream;

/**
 * @class TcpHandler
 *
 * @brief Receive and process data from TCP clients. Each object
 * of this class handles data stream from a TCP connection.
 */
class TcpHandler : public EventHandler {
public:
  TcpHandler(SockStream* stream, Reactor* reactor);
  ~TcpHandler();
  
  virtual void handle_event(Socket handle, EventType et);
  virtual Socket get_handle() const;

protected:
  virtual void handle_read(Socket handle);
  virtual void handle_write(Socket handle);
  virtual void handle_close(Socket handle);
  virtual void handle_except(Socket handle);

private:
  //Receives data from a connected client
  SockStream* sock_stream_;
  
  //Store process-wide Reactor instance
  Reactor* reactor_;
  
  //  struct SipMsgBuff sip_msg_;
};

#endif // TCP_HANDLER_H_
