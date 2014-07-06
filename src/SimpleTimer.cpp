/*
 * =====================================================================================
 *  FILENAME	:  SimpleTimer.c
 *  DESCRIPTION	:  
 *  VERSION		:  1.0
 *  CREATED		:  01/18/2011 10:39:57 AM
 *  REVISION	:  none
 *  COMPILER	:  g++
 *  AUTHOR		:  Ngoc Son 
 *  COPYRIGHT	:  Copyright (c) 2011, Ngoc Son
 *
 * =====================================================================================
 */
#include "SimpleTimer.h"

TimerList* TimerList::mInstance = NULL;

TimerList::TimerList() {
	timer_t timer;

	if(timer_create(CLOCK_REALTIME, NULL, &timer)) {
		perror("timer_create");
	}
	mTimerId = timer;
}

TimerList::~TimerList() {
	timer_delete(mTimerId);
}

TimerList* TimerList::mGetInstance() {
	if(mInstance == NULL) {
		mInstance = new TimerList();
	}
	return mInstance;
}

bool TimerList::mRegisterHandler(TimerHandler handler) {
	if(handler == NULL) {
		return false;
	}

	mHandler = handler;
	return true;
}

bool TimerList::mAdd(int firedTime /* in milisecond */, TimerType type) {
	if(firedTime < MIN_EXPIRE_TIME_MS) {
		return false;
	}

	struct SingleTimer timer;
	timer.mRemainTime = firedTime;
	timer.mType = type;
	mList.push_back(timer);
	return true;
}

void TimerList::mDelete(int index) {

}

void TimerList::mListener(int signo) {
	TimerList* myself = TimerList::mGetInstance();
	list<SingleTimer>* tempList = &(myself->mList);
	list<SingleTimer>::iterator iter;
	
	if(tempList->size() == 0) {
		return;
	}

	for(iter = tempList->begin(); iter != tempList->end(); iter++) {
		iter->mRemainTime = iter->mRemainTime - COMMON_STEP;
		if(iter->mRemainTime <= 0) {
			myself->mHandler(iter->mType);
			//remove element has mRemainTime <= 0
			iter = tempList->erase(iter);
		}
	}
}

void TimerList::mRun() {
	struct itimerspec ts;
	ts.it_interval.tv_sec = INTERVAL_TV_SEC;
	ts.it_interval.tv_nsec = INTERVAL_TV_NSEC; /*COMMON_STEP * 1000000; //Timer fire each 0.25 second.*/
	ts.it_value.tv_sec = INITIAL_TV_SEC;	//current fire time is 5s
	ts.it_value.tv_nsec = INITIAL_TV_NSEC;

	signal(SIGALRM, mListener);
	if(timer_settime(mTimerId, 0, &ts, NULL)) {
		perror("timer_settime");
		return;
	}

	while(mList.size() != 0) {
		pause();
	}
}
