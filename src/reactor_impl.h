/*
 * =====================================================================================
 *  FILENAME	:  ReactorImpl.h
 *  DESCRIPTION	:  
 *  VERSION		:  1.0
 *  CREATED		:  12/03/2010 02:24:11 PM
 *  REVISION	:  none
 *  COMPILER	:  g++
 *  AUTHOR		:  Ngoc Son
 *  COPYRIGHT	:  Copyright (c) 2010, Ngoc Son
 *
 * =====================================================================================
 */
#ifndef REACTOR_IMPL_H_
#define REACTOR_IMPL_H_

#include <stdio.h>
#include <poll.h>
#include <sys/epoll.h>

#include "socket_wf.h"
#include "reactor_type.h"
#include "reactor.h"

class EventHandler;

struct Tuple {
	//pointer to Event_Handler that processes the events arriving
	//on the handle
	EventHandler* mEventHandler;
	//bitmask that tracks which types of events <Event_Handler>
	//is registered for
	EventType mEventType;
};


/*
 * =====================================================================================
 *        Class:  DemuxTable
 *  Description:  Demultiplexing table contains mapping tuples
 *  			  <SOCKET, EventHandler, EventType>. This table maintained by
 *  			  implementing Reactor class so it can dispatch current event with a
 *  			  particular type to appropriate handler.
 * =====================================================================================
 */
class DemuxTable {
	public:
		DemuxTable();
		~DemuxTable();
		void mConvertToFdSets(fd_set &readset, fd_set &writeset, fd_set &exceptset, SOCKET &max_handle);
		
	public:
		//because the number of file descriptors can be demultiplexed by
		//select() is limited by FD_SETSIZE constant so this table is indexed
		//up to FD_SETSIZE
		struct Tuple mTable[FD_SETSIZE];
};


/*
 * =====================================================================================
 *        Class:  ReactorImpl
 *  Description:  Interface for implementation of Reactor. It is abstract base class.
 *  			  User MUST register ReactorStreamHandleRead if they use TCP
 *  			  User MUST register ReactorDgramHandleRead if they use UDP
 *  			  ReactorStreamHandleEvent & ReactorDgramHandleEvent can be registered
 *				  as user's demand.
 *		   Note:  That this pure abstract class just define interfaces for transfering 
 *		   		  read data to callbacks. Transfer event methods to user's callbacks can 
 *		   		  be implemented by derived classes such as SelectReactorImpl,...
 * =====================================================================================
 */
class ReactorImpl {

	public:
		virtual void mRegisterHandler(EventHandler* eh, EventType et)=0;
		virtual void mRegisterHandler(SOCKET h, EventHandler* eh, EventType et)=0;
		virtual void mRemoveHandler(EventHandler* eh, EventType et)=0;
		virtual void mRemoveHandler(SOCKET h, EventType et)=0;
		virtual void mHandleEvents(Time_Value* timeout=NULL)=0;
};

////////////////////////////////////////////////////////
//Following classes are concrete classes which implement
//ReactorImpl interface using select(), poll(), epoll() 
//, kqueue and /dev/poll demultiplexer
////////////////////////////////////////////////////////


/*
 * =====================================================================================
 *        Class:  SelectReactorImpl
 *  Description:  It uses traditional select() function as demultiplexer
 * =====================================================================================
 */
class SelectReactorImpl : public ReactorImpl {
	private:
		DemuxTable mTable;
		fd_set	mRdSet, mWrSet, mExSet;
		int		mMaxHandle;
	
	public:
		SelectReactorImpl();
		~SelectReactorImpl();

		void mRegisterHandler(EventHandler* eh, EventType et);
		void mRegisterHandler(SOCKET h, EventHandler* eh, EventType et);
		void mRemoveHandler(EventHandler* eh, EventType et);
		void mRemoveHandler(SOCKET h, EventType et);
		void mHandleEvents(Time_Value* timeout=NULL);
};


/*
 * =====================================================================================
 *        Class:  PollReactorImpl
 *  Description:  It uses traditional poll() function as demultiplexer
 * =====================================================================================
 */
class PollReactorImpl : public ReactorImpl {
	private:
		struct pollfd	mClient[MAXFD];
		EventHandler*	mHandler[MAXFD];
		int	mMaxi;

	public:
		PollReactorImpl();
		~PollReactorImpl();

		void mRegisterHandler(EventHandler* eh, EventType et);
		void mRegisterHandler(SOCKET h, EventHandler* eh, EventType et);
		void mRemoveHandler(EventHandler* eh, EventType et);
		void mRemoveHandler(SOCKET h, EventType et);
		void mHandleEvents(Time_Value* timeout=NULL);
};

/*
 * =====================================================================================
 *        Class:  DevPollReactorImpl
 *  Description:  It uses /dev/poll as demultiplexer. This class is used with Solaris OS.
 *  NOTE	   :  - handler of file descriptor x is mHandler[x]
 *  			  - mBuf[] is array contains list of pollfd struct are currently observed
 *  			  - mOutput is output of ioctl() function, contains list of pollfds
 *  			  on which events occur.
 * =====================================================================================
 */
#ifdef HAS_DEV_POLL
#include <sys/devpoll.h>

class DevPollReactorImpl : public ReactorImpl {
	private:
		int mDevpollfd;
		struct pollfd mBuf[MAXFD]; //input interested file descriptors
		struct pollfd* mOutput;	   //file descriptors has event
		EventHandler* mHandler[MAXFD]; //keep track of handler for each file descriptor
		
	public:
		DevPollReactorImpl();
		~DevPollReactorImpl();

		void mRegisterHandler(EventHandler* eh, EventType et);
		void mRegisterHandler(SOCKET h, EventHandler* eh, EventType et);
		void mRemoveHandler(EventHandler* eh, EventType et);
		void mRemoveHandler(SOCKET h, EventType et);
		void mHandleEvents(Time_Value* timeout=NULL);
};
#endif

/*
 * =====================================================================================
 *        Class:  EpollReactorImpl
 *  Description:  It uses Linux's epoll() as demultiplexer
 * =====================================================================================
 */
class EpollReactorImpl : public ReactorImpl {
	private:
		int		mEpollFd;
		struct epoll_event	mEvents[MAXFD];//Output from epoll_wait()
		EventHandler*		mHandler[MAXFD];
		
	public:
		EpollReactorImpl();
		~EpollReactorImpl();

		void mRegisterHandler(EventHandler* eh, EventType et);
		void mRegisterHandler(SOCKET h, EventHandler* eh, EventType et);
		void mRemoveHandler(EventHandler* eh, EventType et);
		void mRemoveHandler(SOCKET h, EventType et);
		void mHandleEvents(Time_Value* timeout=NULL);
};

/*
 * =====================================================================================
 *        Class:  KqueueReactorImpl
 *  Description:  It uses BSD-derived kqueue() as demultiplexer. This class
 *  			  used for BSD-derived systems such as FreeBSD, NetBSD
 * =====================================================================================
 */
#ifdef HAS_EVENT_H
#include <sys/event.h>
#include <sys/types.h>

class KqueueReactorImpl : public ReactorImpl {
	private:
		int	mKqueue;	//Kqueue identifier
		int mEventsNo;	//The number of descriptors we are expecting events occur on.

	public:
		KqueueReactorImpl();
		~KqueueReactorImpl();
		
		void mRegisterHandler(EventHandler* eh, EventType et);
		void mRegisterHandler(SOCKET h, EventHandler* eh, EventType et);
		void mRemoveHandler(EventHandler* eh, EventType et);
		void mRemoveHandler(SOCKET h, EventType et);
		void mHandleEvents(Time_Value* timeout=NULL);
};
#endif

#endif // REACTOR_IMPL_H_
