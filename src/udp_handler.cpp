#include "udp_handler.h"

UdpHandler::UdpHandler(const InetAddr& addr, Reactor* reactor) {
  sock_dgram_ = new SockDatagram(addr);
  reactor_ = reactor;
  reactor->register_handler(this, READ_EVENT);
}

UdpHandler::~UdpHandler() {
  // Removing handler need to get socket descriptor from mSockDgram,
  // so we must delete mSockDgram after calling mRemoveHandler.
  reactor_->remove_handler(this, READ_EVENT);
  delete sock_dgram_;
}

void UdpHandler::handle_event(Socket sockfd, EventType et) {
  if((et & READ_EVENT) == READ_EVENT) {
    handle_read(sockfd);
  } else if((et & WRITE_EVENT) == WRITE_EVENT) {
    handle_write(sockfd);
  } else if((et & EXCEPT_EVENT) == EXCEPT_EVENT) {
    handle_except(sockfd);
  }
}

void UdpHandler::handle_read(Socket sockfd) {
  char buff[SIP_UDP_MSG_MAX_SIZE];
  ssize_t n;
  struct sockaddr_in cliaddr;
  socklen_t clilen = sizeof(cliaddr);

  n = sock_dgram_->recv_from(buff, sizeof(buff), 0, (struct sockaddr*)&cliaddr, &clilen);
  if(n < 0){
    return;
  }

  // Check whether end-of-message reach or not. If not reach, may be message is larger than
  // 3kB. We send error response in this case. If reach end of msg, transfer to user's callback
  reactor_->udp_read_handler_(cliaddr, buff, n);
}

void UdpHandler::handle_write(Socket sockfd) {
  // Ignore this method because we only register READ_EVENT to Reactor.
}

void UdpHandler::handle_except(Socket sockfd) {
  // Ignore this method because we only register READ_EVENT to Reactor.
}

Socket UdpHandler::get_handle() const {
  return sock_dgram_->get_handle();
}
