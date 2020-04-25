#include "tcp_handler.h"


TcpHandler::TcpHandler(SockStream* stream, Reactor* reactor) {
  // TODO: can we use assignment operator for reference variable
  sock_stream_ = stream;
  reactor_ = reactor;
  reactor->register_handler(this, READ_EVENT);
  
  memset(sip_msg_.big_buff, 0x00, sizeof(sip_msg_.big_buff));
  sip_msg_.is_reading_body = false;
  sip_msg_.remain_body_len = 0;
}

TcpHandler::~TcpHandler() {
  // Remove itself from demultiplexer table of Reactor
  reactor_->remove_handler(this, READ_EVENT);
  
  // Former action requires socket descriptor which get from mSockStream
  // so we remove this SOCK_Stream object latter
  delete sock_stream_;
}

/**
 * @brief Delegate to the correct handler method.
 */
void TcpHandler::handle_event(Socket h, EventType et) {
  if((et & READ_EVENT) == READ_EVENT) {
    // Save received data to internal buffer and process it
    handle_read(h);
  } else if((et & WRITE_EVENT) == WRITE_EVENT) {
    handle_write(h);
  } else if((et & EXCEPT_EVENT) == EXCEPT_EVENT) {
    // This object is allocated by ConnectionAcceptor object, so
    // we deallocate it here if event is CLOSE
    handle_except(h);
  }
}

Socket TcpHandler::get_handle() const {
  return sock_stream_->get_handle();
}

/**
 * @brief Read message from clients and call to user callback. 
 * In case of TCP, we need to read until to delimiter of data stream.
 */
void TcpHandler::handle_read(Socket handle) {
  ssize_t n;
  int contlen_value;
  bool is_valid = false;   //Check Content-Length value is valid or not
  char buff[TEMP_MSG_SIZE]; //buffer for each time read from kernel
  char* emptyline, *contlen_begin, *contlen_end;
  memset(buff, 0x00, sizeof(buff));

  /////////////////////////////////////////////////////////
  //// We've already read headers. Now we are reading body.
  //// Read until the end of body, save to big buff and
  //// return to user's callback function.
  /////////////////////////////////////////////////////////
  if(sip_msg_.is_reading_body == true){
    if(sip_msg_.remain_body_len > TEMP_MSG_SIZE)
      n = read(handle, buff, sizeof(buff));
    else
      n = read(handle, buff, sip_msg_.remain_body_len);

    if((n < 0 && errno == ECONNRESET) || (n == 0)) {
      handle_close(handle);
      return;
    }
    if(n < sip_msg_.remain_body_len)
      sip_msg_.remain_body_len = sip_msg_.remain_body_len - n;
    strncat(sip_msg_.big_buff, buff, strlen(buff));

    //Call to user's callback function
    //mReactor->mGetReactorImpl()->mTCPReadHandler(handle, mSipMsg.mBigBuff, strlen(mSipMsg.mBigBuff));
    reactor_->tcp_read_handler_(handle, sip_msg_.big_buff, strlen(sip_msg_.big_buff));

    memset(sip_msg_.big_buff, 0x00, sizeof(sip_msg_.big_buff));
    sip_msg_.is_reading_body = false;
    sip_msg_.remain_body_len = 0;

    //////////////////////////////////////////////////////////////
    //// We are still reading headers. If we found empty line,
    //// save these header bytes and move to read body. Else,
    //// save all read header bytes to big buff and continue read
    //////////////////////////////////////////////////////////////
  }else if(sip_msg_.is_reading_body == false){
    n = read(handle, buff, sizeof(buff));
    if((n < 0 && errno == ECONNRESET) || (n == 0)){
      handle_close(handle);
      return;
    }
    if((emptyline = strstr(buff, "\r\n\r\n")) != NULL){
      //      pantheios::log_INFORMATIONAL("Found empty line");
      sip_msg_.is_reading_body = true;
      int m = (emptyline + 4 - &buff[0]);
      strncat(sip_msg_.big_buff, buff, m);
      int l = strlen(sip_msg_.big_buff);
      char inter[l+1];
      strncpy(inter, sip_msg_.big_buff, l);
      inter[l+1] = '\0';

      for(int j = 0; j < l; j++)
        inter[j] = tolower(inter[j]);
        
      if((contlen_begin = strstr(inter, "content-length")) == NULL){
        if((contlen_begin = strstr(inter, "\r\nl")) == NULL){
          //          pantheios::log_INFORMATIONAL("Not found Content-Length header");
          handle_close(handle);
          return;
        }
      }

      //pantheios::log_INFORMATIONAL("Found Content-Length");
      if((contlen_end = strstr(contlen_begin, "\r\n")) != NULL){
        for(int i = 0; i < ((contlen_end-contlen_begin)/sizeof(char)); i++){
          if(isdigit(*(contlen_begin + i))){
            contlen_value = atoi(contlen_begin + i);
            is_valid = true;
            break;
          }         
        }
        if(is_valid == false) {
          //handle invalid Content-Length value case
          //pantheios::log_INFORMATIONAL("Invalid Content Length !!!");
          handle_close(handle);
          return;
        }
      }
      //pantheios::log_INFORMATIONAL("Content-Length: ", pantheios::integer(contlen_value));

      //the number of body bytes in buff[] now is (n-m) bytes.
      //n is the number of read bytes, m is the number of bytes til to end of empty line
      //NOTE: buff[] can also contain a beginning of subsequent message with current message
      //we are reading. So we need to save this message for further reading.
      if(contlen_value > (n-m)){
        strncat(sip_msg_.big_buff, emptyline+4, (n-m));
        sip_msg_.remain_body_len = sip_msg_.remain_body_len - (n-m);
      }else{
        //if (n-m) > contlen_value, it means that there are bytes of other
        //message in buff[], we need to save this bytes for that message.
        strncat(sip_msg_.big_buff, emptyline+4, contlen_value);

        //Call to user's callback function
        //mReactor->mGetReactorImpl()->mTCPReadHandler(handle, mSipMsg.mBigBuff, strlen(mSipMsg.mBigBuff));
        reactor_->tcp_read_handler_(handle, sip_msg_.big_buff, strlen(sip_msg_.big_buff));

        memset(sip_msg_.big_buff, 0x00, sizeof(sip_msg_.big_buff));
        sip_msg_.is_reading_body = false;
        sip_msg_.remain_body_len = 0;
      }
    }else{
      //Empty line is not present in buff[]. Save buff to mBigBuff
      strncat(sip_msg_.big_buff, buff, n);
    }
  }
}


/**
 * @brief Handler for ready-to-write event.
 */
void TcpHandler::handle_write(Socket handle) {
  // Ignore this event because we only register READ_EVENT to Reactor
}

/**
 * @brief Handle close event.
 */
void TcpHandler::handle_close(Socket handle) {
  // NOTE: Do not use "delete this" for class that can be instantiated
  // in stack because it is automatic variable and when program go out
  // of its scope, its destructor is call second time. In that case,
  // the result is undefined.
  delete this;
}

/*
 * @brief Handle other events.
 */
void TcpHandler::handle_except(Socket handle) {
  // Ignore this event because we only register READ_EVENT to Reactor
}
