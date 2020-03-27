/*
 * =====================================================================================
 *  FILENAME	:  ReactorImpl.c
 *  DESCRIPTION	:  
 *  VERSION		:  1.0
 *  CREATED		:  12/03/2010 02:22:46 PM
 *  REVISION	:  none
 *  COMPILER	:  g++
 *  AUTHOR		:  Ngoc Son
 *  COPYRIGHT	:  Copyright (c) 2010, Ngoc Son
 *
 * =====================================================================================
 */
#include "reactor_impl.h"


/*
 *--------------------------------------------------------------------------------------
 *       Class:  SelectReactorImpl
 *      Method:  SelectReactorImpl :: SelectReactorImpl()
 * Description:  Constructor of SelectReactorImpl class
 *--------------------------------------------------------------------------------------
 */
SelectReactorImpl::SelectReactorImpl(){
	FD_ZERO(&mRdSet);
	FD_ZERO(&mWrSet);
	FD_ZERO(&mExSet);
	mMaxHandle = 0;
}

SelectReactorImpl::~SelectReactorImpl(){
}

/*
 *--------------------------------------------------------------------------------------
 *       Class:  SelectReactorImpl
 *      Method:  SelectReactorImpl :: mHandleEvents()
 * Description:  Waiting for events using select() function
 *--------------------------------------------------------------------------------------
 */
void SelectReactorImpl::mHandleEvents(Time_Value* timeout){
	fd_set readset, writeset, exceptset;
	readset = mRdSet;
	writeset = mWrSet;
	exceptset = mExSet;

	int result = select(mMaxHandle+1, &readset, &writeset, &exceptset, timeout);
	if(result < 0){
		//exit(EXIT_FAILURE);
		perror("select() error");
		return;
	}
	//pantheios::log_INFORMATIONAL("The number of ready events: ", pantheios::integer(result));

	for(SOCKET h=0; h<=mMaxHandle; h++){
		//We should check for incoming events in each SOCKET
		if(FD_ISSET(h, &readset)){
			(mTable.mTable[h].mEventHandler)->mHandleEvent(h, READ_EVENT);
			if(--result <= 0)
				break;
			continue;
		}
		if(FD_ISSET(h, &writeset)){
			(mTable.mTable[h].mEventHandler)->mHandleEvent(h, WRITE_EVENT);
			if(--result <= 0)
				break;
			continue;
		}
		if(FD_ISSET(h, &exceptset)){
			(mTable.mTable[h].mEventHandler)->mHandleEvent(h, EXCEPT_EVENT);
			if(--result <= 0)
				break;
			continue;
		}
	}
}

#if 0
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

	if(result < 0)	throw /*handle error here*/;

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
#endif

/*
 *--------------------------------------------------------------------------------------
 *       Class:  SelectReactorImpl
 *      Method:  SelectReactorImpl :: mRegisterHandler()
 * Description:  Each time StreamHandler or ConnectionAcceptor or other EventHandler
 * 				 object is created, it registers to this object to receive favor events
 *--------------------------------------------------------------------------------------
 */
void SelectReactorImpl::mRegisterHandler(EventHandler* eh, EventType et){
	//Get SOCKET associated with this EventHandler object
	SOCKET temp = eh->mGetHandle();
	mTable.mTable[temp].mEventHandler = eh;
	mTable.mTable[temp].mEventType = et;
	
	//set maximum handle value
	if(temp > mMaxHandle)
		mMaxHandle = temp;

	//set appropriate bits to mRdSet, mWrSet and mExSet
	if((et & READ_EVENT) == READ_EVENT)
		FD_SET(temp, &mRdSet);
	if((et & WRITE_EVENT) == WRITE_EVENT)
		FD_SET(temp, &mWrSet);
	if((et & EXCEPT_EVENT) == EXCEPT_EVENT)
		FD_SET(temp, &mExSet);
}

void SelectReactorImpl::mRegisterHandler(SOCKET h, EventHandler* eh, EventType et){
	//This is temporarily unavailable
}

/*
 *--------------------------------------------------------------------------------------
 *       Class:  SelectReactorImpl
 *      Method:  SelectReactorImpl :: mRemoveHandler()
 * Description:  When StreamHandler or ConnectionAcceptor or other EventHandler object
 * 				 is destructed, it deregisters to Reactor so Reactor doesn't dispatch
 * 				 events to it anymore.
 *--------------------------------------------------------------------------------------
 */
void SelectReactorImpl::mRemoveHandler(EventHandler* eh, EventType et){
	SOCKET temp = eh->mGetHandle();
	mTable.mTable[temp].mEventHandler = NULL;
	mTable.mTable[temp].mEventType = 0;

	//clear appropriate bits to mRdSet, mWrSet, and mExSet
	if((et & READ_EVENT) == READ_EVENT)
		FD_CLR(temp, &mRdSet);
	if((et & WRITE_EVENT) == WRITE_EVENT)
		FD_CLR(temp, &mWrSet);
	if((et & EXCEPT_EVENT) == EXCEPT_EVENT)
		FD_CLR(temp, &mExSet);
}

void SelectReactorImpl::mRemoveHandler(SOCKET h, EventType et){
	mTable.mTable[h].mEventHandler = NULL;
	mTable.mTable[h].mEventType = 0;

	//clear appropriate bits to mRdSet, mWrSet, and mExSet
	if((et & READ_EVENT) == READ_EVENT)
		FD_CLR(h, &mRdSet);
	if((et & WRITE_EVENT) == WRITE_EVENT)
		FD_CLR(h, &mWrSet);
	if((et & EXCEPT_EVENT) == EXCEPT_EVENT)
		FD_CLR(h, &mExSet);
}


/*
 *--------------------------------------------------------------------------------------
 *       Class:  DemuxTable
 *      Method:  DemuxTable :: DemuxTable()
 * Description:  Constructor just inits memory
 *--------------------------------------------------------------------------------------
 */
DemuxTable::DemuxTable(){
	memset(mTable, 0x00, FD_SETSIZE * sizeof(struct Tuple));
}

DemuxTable::~DemuxTable(){}

/*
 *--------------------------------------------------------------------------------------
 *       Class:  DemuxTable
 *      Method:  DemuxTable :: mConvertToFdSets()
 * Description:  This method converts array of Tuples to three set of file descriptor
 * 				 before calling demultiplexing function. It used with select().
 *--------------------------------------------------------------------------------------
 */
void DemuxTable::mConvertToFdSets(fd_set &readset, fd_set &writeset, fd_set &exceptset, SOCKET &max_handle){
	for(SOCKET i=0; i<FD_SETSIZE; i++){
		if(mTable[i].mEventHandler != NULL){
			//We are interested in this socket, so
			//set max_handle to this socket descriptor
			max_handle = i;
			if((mTable[i].mEventType & READ_EVENT) == READ_EVENT)
				FD_SET(i, &readset);
			if((mTable[i].mEventType & WRITE_EVENT) == WRITE_EVENT)
				FD_SET(i, &writeset);
			if((mTable[i].mEventType & EXCEPT_EVENT) == EXCEPT_EVENT)
				FD_SET(i, &exceptset);
		}
	}
}


/*
 *--------------------------------------------------------------------------------------
 *       Class:  PollReactorImpl
 *      Method:  PollReactorImpl :: PollReactorImpl()
 * Description:  Constructor which inits input array for poll()
 *--------------------------------------------------------------------------------------
 */
PollReactorImpl::PollReactorImpl(){
	for(int i=0; i<MAXFD; i++){
		mClient[i].fd = -1;
		mClient[i].events = 0;
		mClient[i].revents = 0;
		mHandler[i] = NULL;
	}
	mMaxi = 0;
}

PollReactorImpl::~PollReactorImpl(){
}

/*
 *--------------------------------------------------------------------------------------
 *       Class:  PollReactorImpl
 *      Method:  PollReactorImpl :: mHandleEvents()
 * Description:  Waiting for events using poll() function, then dispatch these events
 * 				 to corresponding EventHandler object.
 *--------------------------------------------------------------------------------------
 */
void PollReactorImpl::mHandleEvents(Time_Value* time){
	int nready, timeout, i;
	if(time == NULL)
		timeout = -1;
	else
		timeout = (time->tv_sec)*1000 + (time->tv_usec)/1000;

	nready = poll(mClient, mMaxi+1, timeout);
	if(nready < 0){
		perror("poll() error");
		return;
	}

	for(i=0; i <= mMaxi; i++){
		if(mClient[i].fd < 0)
			continue;
		
		if((mClient[i].revents & POLLRDNORM)){
//			pantheios::log_INFORMATIONAL("Read event at mClient[", pantheios::integer(i), "]");
			mHandler[i]->mHandleEvent(mClient[i].fd, READ_EVENT);
			if(--nready <= 0)
				break;
		}
		if((mClient[i].revents & POLLWRNORM)){
//			pantheios::log_INFORMATIONAL("Write event at mClient[", pantheios::integer(i), "]");
			mHandler[i]->mHandleEvent(mClient[i].fd, WRITE_EVENT);
			if(--nready <= 0)
				break;
		}		
	}
}

/*
 *--------------------------------------------------------------------------------------
 *       Class:  PollReactorImpl
 *      Method:  PollReactorImpl :: mRegisterHandler()
 * Description:  Used by StreamHandler, ConnectionAcceptor or other further EventHandler
 * 				 object to register its handler.
 *--------------------------------------------------------------------------------------
 */
void PollReactorImpl::mRegisterHandler(EventHandler* eh, EventType et){
	int i;
	for(i=0; i<MAXFD; i++)
		//First element in array has fd=-1 will be fill
		//with new handler. Then we break out of for loop
		if(mClient[i].fd == -1){
			mClient[i].fd = eh->mGetHandle();
			mClient[i].events = 0; //reset events field

			if((et & READ_EVENT) == READ_EVENT)
				mClient[i].events |= POLLRDNORM;
			if((et & WRITE_EVENT) == WRITE_EVENT)
				mClient[i].events |= POLLWRNORM;

			mHandler[i] = eh;
			break;
		}

	if(i == MAXFD){
//		pantheios::log_INFORMATIONAL("Too many client !!!");
		exit(EXIT_FAILURE);
	}

//	pantheios::log_INFORMATIONAL("mMaxi: ", pantheios::integer(mMaxi), ". i: ", pantheios::integer(i));
	if(i > mMaxi)
		mMaxi = i;	
}

void PollReactorImpl::mRegisterHandler(SOCKET h, EventHandler* eh, EventType et){
	//Temporary unavailable
}

/*
 *--------------------------------------------------------------------------------------
 *       Class:  PollReactorImpl
 *      Method:  PollReactorImpl :: mRemoveHandler()
 * Description:  Called by ConnectionAcceptor, StreamHandler or other EventHandler when
 * 				 it is destructed, so events are no longer dispatched to them.
 *--------------------------------------------------------------------------------------
 */
void PollReactorImpl::mRemoveHandler(EventHandler* eh, EventType et){
	for(int i=0; i <= mMaxi; i++){
		if(mClient[i].fd == eh->mGetHandle()){
			mClient[i].fd = -1;
			mClient[i].events = 0;
			mHandler[i] = NULL;
			break;
		}
	}
}

void PollReactorImpl::mRemoveHandler(SOCKET h, EventType et){
	for(int i=0; i<mMaxi; i++){
		if(mClient[i].fd == h){
			mClient[i].fd = -1;
			mHandler[i] = NULL;
			break;
		}
	}
}


/*
 * =====================================================================================
 *        Class:  DevPollReactorImpl
 *  Description:  
 * =====================================================================================
 */
#ifdef HAS_DEV_POLL
DevPollReactorImpl::DevPollReactorImpl(){	
	mDevpollfd = open("/dev/poll", O_RDWR);
	if(mDevpollfd < 0) {
//		pantheios::log_INFORMATIONAL("Cannot open /dev/poll !!!");
		exit(EXIT_FAILURE);
	}

	//init input array of struct pollfd
	for(int i=0; i<MAXFD; i++){
		mBuf[i].fd = -1;
		mBuf[i].events = 0;
		mBuf[i].revents = 0;
		mHandler[i] = NULL;
	}

	//init output array of pollfd has event on
	mOutput = (struct pollfd*) malloc(sizeof(struct pollfd) * MAXFD);
}

DevPollReactorImpl::~DevPollReactorImpl(){
	close(mDevpollfd);
	if(mOutput != NULL) {
		free(mOutput);
	}
}

void DevPollReactorImpl::mHandleEvents(Time_Value* time){
	int nready, timeout, i;
	if(time == NULL) {
		timeout = -1;
	} else {
		timeout = (time->tv_sec)*1000 + (time->tv_usec)/1000;
	}

	struct dvpoll dopoll;
	dopoll.dp_timeout = timeout;
	dopoll.dp_nfds = MAXFD;
	dopoll.dp_fds = mOutput;

	//waiting for events
	nready = ioctl(mDevpollfd, DP_POLL, &dopoll);

	if(nready < 0) {
		perror("/dev/poll ioctl DP_POLL failed");
		return;
	}

	for(i=0; i<nready; i++) {
		int temp = (mOutput+i)->fd;

		if((mOutput + i)->revents & POLLRDNORM) {
			mHandler[temp]->mHandleEvent(temp, READ_EVENT);
		}
		if((mOutput+i)->revents & POLLWRNORM) {
			mHandler[temp]->mHandleEvent(temp, WRITE_EVENT);
		}
	}
}

void DevPollReactorImpl::mRegisterHandler(EventHandler* eh, EventType et){
	int temp = eh->mGetHandle();

	for(int i=0; i<MAXFD; i++) {
		if(mBuf[i].fd < 0) {
			mBuf[i].fd = eh->mGetHandle();
			mBuf[i].events = 0; //reset events field

			if((et & READ_EVENT) == READ_EVENT) {
				mBuf[i].events |= POLLRDNORM;
			}
			if((et & WRITE_EVENT) == WRITE_EVENT) {
				mBuf[i].events |= POLLWRNORM;
			}

			//handler of a particular descriptor is saved to index has the same value with the descriptor
			mHandler[temp] = eh;
			break;
   		}
	}

	if(i == MAXFD) {
//		pantheios::log_INFORMATIONAL("Too many client !");
		exit(EXIT_FAILURE);
	}

	//close old mDevpollfd file and open new file rely on new set of pollfds
	close(mDevpollfd);
	mDevpollfd = open("/dev/poll", O_RDWR);
	if(mDevpollfd < 0) {
		perror("Cannot open /dev/poll !!!");
		exit(EXIT_FAILURE);
	}

	if(write(mDevpollfd, mBuf, (sizeof(struct pollfd) * MAXFD)) != 
										(sizeof(struct pollfd) * MAXFD) ) {
		perror("Failed to write all pollfds");
		close(mDevpollfd);
		if(mOutput != null) {
			free(mOutput);
			mOutput = NULL;
		}
		exit(EXIT_FAILURE);
	}
}

void DevPollReactorImpl::mRegisterHandler(SOCKET h, EventHandler* eh, EventType et){
	//temporarily unavailable
}

void DevPollReactorImpl::mRemoveHandler(EventHandler* eh, EventType et){
	int temp = eh->mGetHandle();
	mRemoveHandler(temp, et);
}

void DevPollReactorImpl::mRemoveHandler(SOCKET h, EventType et){

	//find the pollfd which has the same file descriptor
	for(int i=0; i<MAXFD; i++) {
		if(mBuf[i].fd == h) {
			mBuf[i].fd = -1;
			mBuf[i].events = 0;
			mHandler[h] = NULL;
			break;
		}
	}

	//rewrite the /dev/poll
	close(mDevpollfd);
	mDevpollfd = open("/dev/poll", O_RDWR);
	if(mDevpollfd < 0) {
		perror("Cannot open /dev/poll !!!");
		exit(EXIT_FAILURE);
	}

	if(write(mDevpollfd, mBuf, (sizeof(struct pollfd) * MAXFD)) != 
										(sizeof(struct pollfd) * MAXFD) ) {
		perror("Failed to write all pollfds");
		close(mDevpollfd);
		if(mOutput != null) {
			free(mOutput);
			mOutput = NULL;
		}
		exit(EXIT_FAILURE);
	}
}
#endif


/*
 * =====================================================================================
 *        Class:  EpollReactorImpl
 *  Description:  Reactor implementation using epoll() function.
 * =====================================================================================
 */

/*
 *--------------------------------------------------------------------------------------
 *       Class:  EpollReactorImpl
 *      Method:  EpollReactorImpl :: EpollReactorImpl()
 * Description:  Constructor which create a new epoll instance
 *--------------------------------------------------------------------------------------
 */
EpollReactorImpl::EpollReactorImpl(){
	for(int i=0; i<MAXFD; i++){
		mHandler[i] = NULL;
	}

	mEpollFd = epoll_create(MAXFD);
	if(mEpollFd < 0){
		perror("epoll_create");
		exit(EXIT_FAILURE);
	}
}

EpollReactorImpl::~EpollReactorImpl(){
}

/*
 *--------------------------------------------------------------------------------------
 *       Class:  EpollReactorImpl
 *      Method:  EpollReactorImpl :: mRegisterHandler()
 * Description:  EventHandler objects register themselves to EpollReactorImpl through
 * 				 this method.
 * 		  Note:  This class only works well if the file descriptor assigned by kernel 
 * 		  		 each time a socket is created is lowest one available. It means that
 * 		  		 it only accepts socket with descriptor number < MAXFD. Ouch !!!
 *--------------------------------------------------------------------------------------
 */
void EpollReactorImpl::mRegisterHandler(EventHandler* eh, EventType et){
	SOCKET sockfd = eh->mGetHandle();
	if(sockfd >= MAXFD){
//		pantheios::log_INFORMATIONAL("Too large file descriptor !!!");
		return;
	}
	
	struct epoll_event addEvent;
	addEvent.data.fd = sockfd;
	addEvent.events = 0; //reset registered events

	if((et & READ_EVENT) == READ_EVENT)		
		addEvent.events |= EPOLLIN;
	if((et & WRITE_EVENT) == WRITE_EVENT)
		addEvent.events |= EPOLLOUT;
	
	if(epoll_ctl(mEpollFd, EPOLL_CTL_ADD, sockfd, &addEvent) < 0){
		perror("epoll_ctl ADD");
		return;
	}
	mHandler[sockfd] = eh;
}

void EpollReactorImpl::mRegisterHandler(SOCKET h, EventHandler* eh, EventType et){
	//temporarily unavailable
}

/*
 *--------------------------------------------------------------------------------------
 *       Class:  EpollReactorImpl
 *      Method:  EpollReactorImpl :: mRemoveHandler()
 * Description:  EventHandler objects deregister themselves to EpollReactorImpl through
 * 				 this method.
 *--------------------------------------------------------------------------------------
 */
void EpollReactorImpl::mRemoveHandler(EventHandler* eh, EventType et){
	SOCKET sockfd = eh->mGetHandle();
	if(sockfd >= MAXFD){
//		pantheios::log_INFORMATIONAL("Bad file descriptor !!!");
		return;
	}
	
	struct epoll_event rmEvent;
	rmEvent.data.fd = sockfd;
	if((et & READ_EVENT) == READ_EVENT)
		rmEvent.events |= EPOLLIN;
	if((et & WRITE_EVENT) == WRITE_EVENT)
		rmEvent.events |= EPOLLOUT;

	if(epoll_ctl(mEpollFd, EPOLL_CTL_DEL, sockfd, &rmEvent) < 0){
		perror("epoll_ctl DEL");
		return;
	}

	mHandler[sockfd] = NULL;
}

void EpollReactorImpl::mRemoveHandler(SOCKET h, EventType et){
	//temporarily unavailable
}

/*
 *--------------------------------------------------------------------------------------
 *       Class:  EpollReactorImpl
 *      Method:  EpollReactorImpl :: mHandleEvents()
 * Description:  Waiting for events using epoll_wait() function.
 *--------------------------------------------------------------------------------------
 */
void EpollReactorImpl::mHandleEvents(Time_Value* time){
	int nevents;
	SOCKET temp;
	int timeout;

	if(time == NULL) {
		timeout = -1;
	} else {
		timeout = (time->tv_sec)*1000 + (time->tv_usec)/1000;
	}

	nevents = epoll_wait(mEpollFd, mEvents, MAXFD, timeout);
	if(nevents < 0){
		perror("epoll_wait");
		return;
	}

	for(int i=0; i<nevents; i++){
		temp = mEvents[i].data.fd;
		if((mEvents[i].events & EPOLLIN) == EPOLLIN)
			mHandler[temp]->mHandleEvent(temp, READ_EVENT);

		//////////////////////////////////////////////////////////////////////////////////////////
		//NOTE: be careful in following case
		// - EventHandler register both EPOLLIN and EPOLLOUT event
		// - Client associated with that EventHandler close connection
		// - EPOLLIN & EPOLLOUT events return from epoll_wait()
		// - Upon processing EPOLLIN event, we free EventHandler (because connection is closed)
		// and set mHandler[i] to NULL.
		// - Afterward, EPOLLOUT event is processed, mHandleEvent() is call,
		// but mHandler[i] now = NULL, so this cause SEGMENTATION FAULT !!!!!!
		//
		// =>> One solution is: Process EPOLLOUT first, before EPOLLIN is processed.
		//////////////////////////////////////////////////////////////////////////////////////////
		if((mEvents[i].events & EPOLLOUT) == EPOLLOUT)
			mHandler[temp]->mHandleEvent(temp, WRITE_EVENT);
	}
}


/*
 * =====================================================================================
 *        Class:  KqueueReactorImpl
 *  Description:  This is implementation of Reactor with FreeBSD's kqueue facility.
 *  	   Note:  struct kevent {
 *  					uintptr_t	ident;	//it is often file descriptor
 *  					short		filter;	//flag indicates filter for above fd
 *  					u_short		flags;	//flag indicates action with above fd or
 *  										//returned values from kevent()
 *  										
 *  					u_int		fflags;	//it is filter flags when registering or
 *  										//returned events retrieved by user
 *
 *  					intptr_t	data;	//data of filter
 *  					void*		udata;  //user data
 *  			  };
 *
 * 	Predefined filters: EVFILT_READ, EVFILT_WRITE, EVFILT_AIO, EVFILT_VNODE, 
 * 						EVFILT_PROC, EVFILT_SIGNAL
 * 		   Input flags: EV_ADD, EV_DELETE, EV_ENABLE, EV_DISABLE, EV_CLEAR, EV_ONESHOT
 * 		  Output flags: EV_EOF, EV_ERROR
 *		  Filter flags: (EVFILT_VNODE) NOTE_DELETE, NOTE_WRITE, NOTE_EXTEND, NOTE_ATTRIB,
 *		  				NOTE_LINK, NOTE_RENAME
 *		  				(EVFILT_PROC) NOTE_EXIT, NOTE_FORK, NOTE_EXEC, NOTE_TRACK,
 *		  				NOTE_CHILD, NOTE_TRACKERR
 *
 * =====================================================================================
 */
#ifdef HAS_EVENT_H

/*
 *--------------------------------------------------------------------------------------
 *       Class:  KqueueReactorImpl
 *      Method:  KqueueReactorImpl :: KqueueReactorImpl()
 * Description:  Constructor creates new kqueue instance.
 *--------------------------------------------------------------------------------------
 */
KqueueReactorImpl::KqueueReactorImpl(){
	mEventsNo = 0;

	if((mKqueue = kqueue()) == -1){
		perror("kqueue() error");
		exit(EXIT_FAILURE);
	}
}

KqueueReactorImpl::~KqueueReactorImpl(){
}

/*
 *--------------------------------------------------------------------------------------
 *       Class:  KqueueReactorImpl
 *      Method:  KqueueReactorImpl :: mRegisterHandler()
 * Description:  Register new socket descriptor to kqueue. Called when new socket is
 * 				 created.
 *--------------------------------------------------------------------------------------
 */
void KqueueReactorImpl::mRegisterHandler(EventHandler* eh, EventType et){
	SOCKET temp = eh->mGetHandle();
	struct kevent event;
	u_short flags = 0;

	if((et & READ_EVENT) == READ_EVENT)
		flags |= EVFILT_READ;
	if((et & WRITE_EVENT) == WRITE_EVENT)
		flags |= EVFILT_WRITE;

	//we set pointer to EventHandler to event, so we have handler associated with
	//that socket when receive return from kevent() function.
	//Prototype:
	//	EV_SET(&kev, ident, filter, flags, fflags, data, udata);
	///////////////
	
	EV_SET(&event, temp, flags, EV_ADD | EV_ENABLE, 0, 0, eh);

	if(kevent(mKqueue, &event, 1, NULL, 0, NULL) == -1){
		perror("kevent() error");
		return;
	}

	mEventsNo++;
}

void KqueueReactorImpl::mRegisterHandler(SOCKET h, EventHandler* eh, EventType et){
}

/*
 *--------------------------------------------------------------------------------------
 *       Class:  KqueueReactorImpl
 *      Method:  KqueueReactorImpl :: mRemoveHandler()
 * Description:  Remove existing socket in kqueue. 
 *--------------------------------------------------------------------------------------
 */
void KqueueReactorImpl::mRemoveHandler(EventHandler* eh, EventType et){
	SOCKET temp = eh->mGetHandle();
	struct kevent event;
	u_short flags = 0;

	if((et & READ_EVENT) == READ_EVENT)
		flags |= EVFILT_READ;
	if((et & WRITE_EVENT) == WRITE_EVENT)
		flags |= EVFILT_WRITE;

	EV_SET(&event, temp, flags, EV_DELETE, 0, 0, 0);

	if(kevent(mKqueue, &event, 1, NULL, 0, NULL) == -1){
		perror("kevent() error");
		return;
	}

	mEventsNo--;
}

void KqueueReactorImpl::mRemoveHandler(SOCKET h, EventType et){
}

/*
 *--------------------------------------------------------------------------------------
 *       Class:  KqueueReactorImpl
 *      Method:  KqueueReactorImpl :: mHandleEvents()
 * Description:  Waiting for events come to sockets in kqueue
 *--------------------------------------------------------------------------------------
 */
void KqueueReactorImpl::mHandleEvents(Time_Value* time){
	int nevents;
	struct kevent ev[mEventNo];
	struct timespec* tout;

	if(time == NULL)
		tout = NULL;
	else{
		tout = new timespec;
		tout->tv_sec = time->tv_sec;
		tout->tv_nsec = time->tv_usec*1000;
	}

	nevents = kevent(mKqueue, NULL, 0, ev, mEventsNo, tout);
	if(nevents < 0){
		if(tout != NULL) 
			delete tout;
		perror("kevent() error");
		return;
	}
	
	if(tout != NULL)
		delete tout;

	for(int i=0; i<nevents; i++){
		if(ev[i].filter == EVFILT_READ)
			(EventHandler*)ev[i].udata->mHandleEvent(ev[i].ident, READ_EVENT);
		if(ev[i].filter == EVFILT_WRITE)
			(EventHandler*)ev[i].udata->mHandleEvent(ev[i].ident, WRITE_EVENT);
	}
}

#endif
