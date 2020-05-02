#include <iostream>

#include "reactor_impl.h"

DemuxTable::DemuxTable() {
  memset(table_, 0x00, FD_SETSIZE * sizeof(struct Tuple));
}

DemuxTable::~DemuxTable() {
}

/**
 * @brief This method converts array of Tuples to three set of file descriptor
 * before calling demultiplexing function. It is used with select().
 */
void DemuxTable::convert_to_fd_sets(fd_set &readset, fd_set &writeset,
                                    fd_set &exceptset, Socket &max_handle) {
  for (Socket i = 0; i < FD_SETSIZE; i++) {
    if (table_[i].event_handler != nullptr) {
      // We are interested in this socket, so
      // set max_handle to this socket descriptor
      max_handle = i;
      if ((table_[i].event_type & READ_EVENT) == READ_EVENT)
        FD_SET(i, &readset);
      if ((table_[i].event_type & WRITE_EVENT) == WRITE_EVENT)
        FD_SET(i, &writeset);
      if ((table_[i].event_type & EXCEPT_EVENT) == EXCEPT_EVENT)
        FD_SET(i, &exceptset);
    }
  }
}
