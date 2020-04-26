#include "reactor_impl.h"

SelectReactorImpl::SelectReactorImpl() {
  FD_ZERO(&rdset_);
  FD_ZERO(&wrset_);
  FD_ZERO(&exset_);
  max_handle_ = 0;
}

SelectReactorImpl::~SelectReactorImpl() {
}

/**
 * @brief Waiting for events using select.
 */
void SelectReactorImpl::handle_events(TimeValue* timeout) {
  fd_set readset, writeset, exceptset;
  readset = rdset_;
  writeset = wrset_;
  exceptset = exset_;

  int result = select(max_handle_+1, &readset, &writeset, &exceptset, timeout);
  if (result < 0) {
    //exit(EXIT_FAILURE);
    perror("select() error");
    return;
  }

  for (Socket h = 0; h <= max_handle_; h++) {
    //We should check for incoming events in each SOCKET
    if (FD_ISSET(h, &readset)) {
      (table_.table_[h].event_handler)->handle_event(h, READ_EVENT);
      if (--result <= 0)
        break;
      continue;
    }
    
    if (FD_ISSET(h, &writeset)) {
      (table_.table_[h].event_handler)->handle_event(h, WRITE_EVENT);
      if (--result <= 0)
        break;
      continue;
    }
    
    if (FD_ISSET(h, &exceptset)) {
      (table_.table_[h].event_handler)->handle_event(h, EXCEPT_EVENT);
      if (--result <= 0)
        break;
      continue;
    }
  }
}

/*
void SelectReactorImpl::mHandleEvents(Time_Value* timeout){
  SOCKET max_handle;
  fd_set readset, writeset, exceptset;
  FD_ZERO(&readset);
  FD_ZERO(&writeset);
  FD_ZERO(&exceptset);
  mTable.mConvertToFdSets(readset, writeset, exceptset, max_handle);
  
  pantheios::log_INFORMATIONAL("Max_handle: ", pantheios::integer(max_handle), ". Demultiplex using select()");
  int result = select(max_handle+1, &readset, &writeset, &exceptset, timeout);
  pantheios::log_INFORMATIONAL("Result after select(): ", pantheios::integer(result));

  if(result < 0)  throw //handle error here

  for(SOCKET h=0; h<=max_handle; h++){
    //We should check for incoming events in each SOCKET
    if(FD_ISSET(h, &readset)){
      (mTable.mTable[h].mEventHandler)->mHandleEvent(h, READ_EVENT);
      continue;
    }
    if(FD_ISSET(h, &writeset)){
      (mTable.mTable[h].mEventHandler)->mHandleEvent(h, WRITE_EVENT);
      continue;
    }
    if(FD_ISSET(h, &exceptset)){
      (mTable.mTable[h].mEventHandler)->mHandleEvent(h, EXCEPT_EVENT);
      continue;
    }
  }
}
*/

/**
 * @brief Concrete event handlers call this method to register.
 */
void SelectReactorImpl::register_handler(EventHandler* eh, EventType et) {
  // Get SOCKET associated with this EventHandler object
  Socket temp = eh->get_handle();
  table_.table_[temp].event_handler = eh;
  table_.table_[temp].event_type = et;
  
  // Set maximum handle value
  if (temp > max_handle_)
    max_handle_ = temp;

  // Set appropriate bits to mRdSet, mWrSet and mExSet
  if ((et & READ_EVENT) == READ_EVENT)
    FD_SET(temp, &rdset_);
  if ((et & WRITE_EVENT) == WRITE_EVENT)
    FD_SET(temp, &wrset_);
  if ((et & EXCEPT_EVENT) == EXCEPT_EVENT)
    FD_SET(temp, &exset_);
}

void SelectReactorImpl::register_handler(Socket h, EventHandler* eh, EventType et) {
  // This is temporarily unavailable
}

/**
 * @brief When an event handler is destructed, it deregisters 
 * to Reactor so Reactor doesn't dispatch events to it anymore.
 */
void SelectReactorImpl::remove_handler(EventHandler* eh, EventType et) {
  Socket temp = eh->get_handle();
  table_.table_[temp].event_handler = nullptr;
  table_.table_[temp].event_type = 0;

  // Clear appropriate bits to mRdSet, mWrSet, and mExSet
  if ((et & READ_EVENT) == READ_EVENT)
    FD_CLR(temp, &rdset_);
  if ((et & WRITE_EVENT) == WRITE_EVENT)
    FD_CLR(temp, &wrset_);
  if ((et & EXCEPT_EVENT) == EXCEPT_EVENT)
    FD_CLR(temp, &exset_);
}

void SelectReactorImpl::remove_handler(Socket h, EventType et) {
  table_.table_[h].event_handler = nullptr;
  table_.table_[h].event_type = 0;

  // Clear appropriate bits to mRdSet, mWrSet, and mExSet
  if ((et & READ_EVENT) == READ_EVENT)
    FD_CLR(h, &rdset_);
  if ((et & WRITE_EVENT) == WRITE_EVENT)
    FD_CLR(h, &wrset_);
  if ((et & EXCEPT_EVENT) == EXCEPT_EVENT)
    FD_CLR(h, &exset_);
}
