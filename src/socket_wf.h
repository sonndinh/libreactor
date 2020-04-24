/*
 * =====================================================================================
 *  FILENAME  :  SocketWF.h
 *  DESCRIPTION :  This file defines Wrapper Facades classes for Socket APIs.
 *           It is used to hide some UNIX/POSIX and Win32 Socket APIs.
 *           Classes:
 *     - INET_Addr: encapsulates IPv4 address
 *       - SOCK_Acceptor: connection factory for TCP. It encapsulates listening
 *       socket and waiting for request connection from clients. A server just needs
 *       one SOCK_Acceptor object for listening to clients on a particular address.
 *     - SOCK_Stream: encapsulates connected TCP descriptor and some function
 *       for sending and receiving data through that descriptor. Each connected
 *       socket corresponds to a SOCK_Stream object.
 *     -SOCK_Datagram: encapsulates UDP socket. Server app just needs to instantiate
 *      one SOCK_Datagram object for listening all clients on one address.
 *
 *  VERSION   :  1.0
 *  CREATED   :  12/03/2010 02:25:17 PM
 *  REVISION  :  none
 *  COMPILER  :  g++
 *  AUTHOR    :  Ngoc Son
 *  COPYRIGHT :  Copyright (c) 2010, Ngoc Son
 *
 * =====================================================================================
 */
#ifndef SOCKET_WF_H_
#define SOCKET_WF_H_

#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>

#include "reactor_type.h"


/*
 * =====================================================================================
 *        Class:  INET_Addr
 *  Description:  INET_Addr class encapsulates Internet address structure
 * =====================================================================================
 */
class InetAddr { 
public:
  InetAddr(uint16_t port){
    memset(&addr_, 0x00, sizeof(addr_));
    addr_.sin_family = AF_INET;
    addr_.sin_port = htons(port);
    addr_.sin_addr.s_addr = INADDR_ANY; //listen on all interfaces
  }
  
  InetAddr(uint16_t port, uint32_t addr) {
    memset(&addr_, 0x00, sizeof(addr_));
    addr_.sin_family = AF_INET; //choose Internet address
    addr_.sin_port = htons(port);
    addr_.sin_addr.s_addr = htonl(addr); //listen on a particular interfaces
  }
  
  uint16_t get_port() {
    return addr_.sin_port;
  }
  
  uint32_t get_ip_addr() {
    return addr_.sin_addr.s_addr;
  }
  
  struct sockaddr* get_addr() const{
    //return reinterpret_cast<struct sockaddr*> (&mAddr);
    return (struct sockaddr*)&addr_;
  }
  
  socklen_t get_size() const {
    return sizeof(addr_);
  }

private:
  struct sockaddr_in addr_; //IPv4 structure
};


/*
 * =====================================================================================
 *        Class:  SOCK_Stream
 *  Description:  SOCK_Stream class encapsulates the I/O operations that an 
 *          application can invoke on a connected socket handle. 
 *          Each SOCK_Stream object is correspond to a TCP connection.
 * =====================================================================================
 */
class SockStream {
public:
  SockStream() {
    //set default handle_ to -1
    handle_ = INVALID_HANDLE_VALUE;
  }

  SockStream(Socket h) {
    handle_ = h;
  }

  //Automatically close the handle on destructor
  ~SockStream() {
    close(handle_);
  }

  //Set the Socket handle
  void set_handle(Socket connsock) {
    handle_ = connsock;
  }

  //Set connected socket descriptor and peer address structure
  void set_peer(Socket connsock, struct sockaddr_in* cliaddr, socklen_t clilen){
    handle_ = connsock;
    memcpy(&peer_addr_, cliaddr, sizeof(peer_addr_));
    peer_addr_len_ = clilen;
  }

  //Get the Socket handle
  Socket get_handle() const {
    return handle_;
  }

  //Normal I/O operations
  ssize_t recv(void* buf, size_t len, int flags);
  ssize_t send(const char* buf, size_t len, int flags);

  //I/O operations for short receives and sends
  ssize_t recv_n(char* buf, size_t len, int flags);
  ssize_t send_n(const char* buf, size_t len, int flags);

  //Other methods

private:
  //It is connected socket
  Socket handle_;
  struct sockaddr_in peer_addr_;
  socklen_t peer_addr_len_;
};


/*
 * =====================================================================================
 *        Class:  SOCK_Acceptor
 *  Description:  SOCK_Acceptor is connection factory which accept connect from client
 *          and initializes SOCK_Stream object for further I/O operations.
 *          This is implementation of TCP listening socket.
 * =====================================================================================
 */
class SockAcceptor {
public:
  //Constructor initializes listenning socket
  SockAcceptor(const InetAddr& addr) {
    //create server socket, use streaming socket (TCP)
    handle_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    //bind between server socket and Internet address
    bind(handle_, addr.get_addr(), addr.get_size());
    //change server socket to listenning mode
    listen(handle_, BACKLOG);
  }

  //A second method to initialize a passive-mode acceptor
  //socket, analogously to the constructor
  void open(const InetAddr &sock_addr) {
  }

  //Accept a connection and initialize the SOCK_Stream
  void accept_sock(SockStream* stream) {
    struct sockaddr_in cliaddr;
    socklen_t clilen = sizeof(cliaddr);

    Socket conn = accept(handle_, (struct sockaddr*)&cliaddr, &clilen);
    //stream->mSetHandle(conn);
    stream->set_peer(conn, &cliaddr, clilen);
    //      pantheios::log_INFORMATIONAL("Set SOCK_Stream with Connection FD: ", pantheios::integer(conn));
  }

  Socket get_handle() const {
    return handle_;
  }

private:
  //Socket handle factory. It is listening socket
  Socket handle_;
};

//SOCK_Acceptor handles factory enables a ConnectionAcceptor object
//to accept connection indications on a passive-mode socket handle that is
//listening on a transport endpoint. When a connection arrives from a client,
//the SOCK_Acceptor accepts the connection passively and produces an initialized
//SOCK_Stream. The SOCK_Stream is then uses TCP to transfer data reliably between 
//the client and the server.



/*
 * =====================================================================================
 *        Class:  SOCK_Datagram
 *  Description:  This class used as wrapper for datagram transport protocol such as UDP
 * =====================================================================================
 */
class SockDatagram {
public:
  SockDatagram(const InetAddr& addr){
    handle_ = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    bind(handle_, addr.get_addr(), addr.get_size());
  }
    
  Socket get_handle() const{
    return handle_;
  }

  ssize_t recv_from(void* buff, size_t nbytes, int flags, struct sockaddr* from, socklen_t* len){
    return recvfrom(handle_, buff, nbytes, flags, from, len);
  }

  ssize_t send_to(const void* buff, size_t nbytes, int flags, const struct sockaddr* to, socklen_t len){
    return sendto(handle_, buff, nbytes, flags, to, len);
  }

private:
  Socket handle_;
};

#endif // SOCKET_WF_H_
