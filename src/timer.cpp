/*
 * =====================================================================================
 *  FILENAME  :  SimpleTimer.c
 *  DESCRIPTION :  
 *  VERSION   :  1.0
 *  CREATED   :  01/18/2011 10:39:57 AM
 *  REVISION  :  none
 *  COMPILER  :  g++
 *  AUTHOR    :  Ngoc Son 
 *  COPYRIGHT :  Copyright (c) 2011, Ngoc Son
 *
 * =====================================================================================
 */
#include "timer.h"

TimerList* TimerList::instance_ = nullptr;

TimerList::TimerList() {
  timer_t timer;

  if(timer_create(CLOCK_REALTIME, nullptr, &timer)) {
    perror("timer_create");
  }
  timer_id_ = timer;
}

TimerList::~TimerList() {
  timer_delete(timer_id_);
}

TimerList* TimerList::get_instance() {
  if(instance_ == nullptr) {
    instance_ = new TimerList();
  }
  return instance_;
}

bool TimerList::register_handler(TimerHandler handler) {
  if(handler == nullptr) {
    return false;
  }

  handler_ = handler;
  return true;
}

bool TimerList::add(int fired_time /* in milisecond */, TimerType type) {
  if(firedTime < MIN_EXPIRE_TIME_MS) {
    return false;
  }

  struct SingleTimer timer;
  timer.remain_time = fired_time;
  timer.type = type;
  list_.push_back(timer);
  return true;
}

void TimerList::remove(int index) {

}

void TimerList::listener(int signo) {
  TimerList* myself = TimerList::get_instance();
  list<SingleTimer>* temp_list = &(myself->list_);
  list<SingleTimer>::iterator iter;
  
  if(temp_list->size() == 0) {
    return;
  }

  for(iter = temp_list->begin(); iter != temp_list->end(); iter++) {
    iter->remain_time = iter->remain_time - COMMON_STEP;
    if(iter->remain_time <= 0) {
      myself->handler_(iter->type);
      //remove element has mRemainTime <= 0
      iter = temp_list->erase(iter);
    }
  }
}

void TimerList::run() {
  struct itimerspec ts;
  ts.it_interval.tv_sec = INTERVAL_TV_SEC;
  ts.it_interval.tv_nsec = INTERVAL_TV_NSEC; /*COMMON_STEP * 1000000; //Timer fire each 0.25 second.*/
  ts.it_value.tv_sec = INITIAL_TV_SEC;  //current fire time is 5s
  ts.it_value.tv_nsec = INITIAL_TV_NSEC;

  signal(SIGALRM, listener);
  if(timer_settime(timer_id_, 0, &ts, NULL)) {
    perror("timer_settime");
    return;
  }

  while(list_.size() != 0) {
    pause();
  }
}
