#ifndef COMMON_H_
#define COMMON_H_

#include <sys/time.h>

const unsigned int MAXFD = 10000; //this is the number of pollfd as input to poll()
const unsigned int BACKLOG = 1000;
const unsigned int SIP_MSG_MAX_SIZE = 64*1024; //maximum size of SIP message we accept
const unsigned int SIP_UDP_MSG_MAX_SIZE = 3*1024; //maximum size of SIP message through UDP
const unsigned int TEMP_MSG_SIZE = 1024;  //Length of data each time read() from kernel


//use 16bits integer as bitmask to point out some considering events
typedef unsigned short EventType;
//redefine file descriptor
typedef unsigned int Handle;
typedef int Socket; //file descriptor for socket
const int INVALID_HANDLE_VALUE = -1;
typedef struct timeval TimeValue;

enum {
      ACCEPT_EVENT = 0x0001,
      READ_EVENT = 0x0002,
      WRITE_EVENT = 0x0004,
      EXCEPT_EVENT = 0x0008,
      TIMEOUT_EVENT = 0x0010,
      SIGNAL_EVENT = 0x0020,
      CLOSE_EVENT = 0x0040
};

//choose demultiplexing mechanism
typedef enum {
              SELECT_DEMUX, //traditional select() function
              POLL_DEMUX,   //traditional poll() function
              DEVPOLL_DEMUX,  //Solaris /dev/poll facility
              EPOLL_DEMUX,  //Linux epoll() function set
              KQUEUE_DEMUX  //FreeBSD, NetBSD kqueue facility
} DemuxType;

typedef enum {
              TCPStateINIT,
              TCPStateCONNECTED,
              TCPStateLISTEN,
              TCPStateCLOSE,
              TCPStateABORT,
              TCPStateCANCEL,
              TCPStateOVERFLOW,
              TCPStateFDMAX,
              TCPStateRSVD,
              TCPStateBADDATA
} TcpState;

typedef enum {
              UDPStateINIT,
              UDPStateLISTEN
} UdpState;


//User's callback functions for Stream transport protocol (TCP) events
typedef void (*ReactorStreamHandleRead)(Socket socket, char* message, size_t msglen);
typedef void (*ReactorStreamHandleEvent)(Socket socket, TcpState state);

//User's callback functions for Datagram transport protocol (UDP) events
typedef void (*ReactorDgramHandleRead)(struct sockaddr_in peeraddr, char* message, size_t msglen);
typedef void (*ReactorDgramHandleEvent)(UdpState state);

//User's callback functions for timer events
typedef void (*ReactorHandleTimer)();

struct SipMsgBuff{
  char big_buff[SIP_MSG_MAX_SIZE];
  bool is_reading_body;
  int remain_body_len;
};


#endif // COMMON_H_
