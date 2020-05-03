#include "connection_acceptor.h"
#include "tcp_handler.h"

ConnectionAcceptor::ConnectionAcceptor(const InetAddr &addr, Reactor* reactor) {
  reactor_ = reactor;
  sock_acceptor_ = new SockAcceptor(addr);

  //Because connection request from client is also READ_EVENT,
  //so we register the event for this object to Reactor
  reactor->register_handler(this, READ_EVENT);
}

ConnectionAcceptor::~ConnectionAcceptor() {
  //Remove this handler from Reactor's Demux table
  reactor_->remove_handler(this, READ_EVENT);
  delete sock_acceptor_;
}

/**
 * @brief Handle connection requests from clients.
 */
void ConnectionAcceptor::handle_event(Socket h, EventType et) {
  if ((et & READ_EVENT) == READ_EVENT){
    // Init new SOCK_Stream with invalid handle.
    // It is freed in TcpHandler's destructor
    SockStream* client = new SockStream();

    // Call accept() to accept connections from clients
    // and set valid handle for SOCK_Stream
    sock_acceptor_->accept_sock(client);
    
    // Freed when client close the connection (FIN is sent)
    TcpHandler* handler = new TcpHandler(client, reactor_);
  }
}

Socket ConnectionAcceptor::get_handle() const {
  return sock_acceptor_->get_handle();
}
