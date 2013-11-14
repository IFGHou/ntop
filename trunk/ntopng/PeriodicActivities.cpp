/*
 *
 * (C) 2013 - ntop.org
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */

#include "ntop_includes.h"

/* ******************************************* */

PeriodicActivities::PeriodicActivities() {

}

/* ******************************************* */

PeriodicActivities::~PeriodicActivities() {
  void *res;

  pthread_join(secondLoop, &res);
  pthread_join(minuteLoop, &res);
  pthread_join(hourLoop, &res);
  pthread_join(dayLoop, &res);
}

/* **************************************************** */

static void* secondStartLoop(void* ptr) {  ((PeriodicActivities*)ptr)->secondActivitiesLoop(); return(NULL); }
static void* minuteStartLoop(void* ptr) {  ((PeriodicActivities*)ptr)->minuteActivitiesLoop(); return(NULL); }
static void* hourStartLoop(void* ptr)   {  ((PeriodicActivities*)ptr)->hourActivitiesLoop(); return(NULL);   }
static void* dayStartLoop(void* ptr)    {  ((PeriodicActivities*)ptr)->dayActivitiesLoop(); return(NULL);    }

/* ******************************************* */

void PeriodicActivities::startPeriodicActivitiesLoop() {
  struct stat buf;

  ntop->getTrace()->traceEvent(TRACE_NORMAL, "Started periodic activities loop...");

  if(stat(ntop->get_callbacks_dir(), &buf) != 0) {
    ntop->getTrace()->traceEvent(TRACE_ERROR, "Unable to read directory %s", ntop->get_callbacks_dir());
    ntop->getTrace()->traceEvent(TRACE_ERROR, "Possible cause:\n");
    ntop->getTrace()->traceEvent(TRACE_ERROR, "The current user cannot access %s.", ntop->get_callbacks_dir());
    ntop->getTrace()->traceEvent(TRACE_ERROR, "Please fix the directory right or add --dont-change-user to");
    ntop->getTrace()->traceEvent(TRACE_ERROR, "the ntopng command line.");
    exit(0);
  }

  pthread_create(&secondLoop, NULL, secondStartLoop, (void*)this);
  pthread_create(&minuteLoop, NULL, minuteStartLoop, (void*)this);
  pthread_create(&hourLoop,   NULL, hourStartLoop,   (void*)this);
  pthread_create(&dayLoop,    NULL, dayStartLoop,    (void*)this);
}

/* ******************************************* */

void PeriodicActivities::runScript(char *path) {
  struct stat statbuf;

  if(stat(path, &statbuf) == 0) {
    Lua *l;

    try {
      l = new Lua();
    } catch(std::bad_alloc& ba) {
      static bool oom_warning_sent = false;
      
      if(!oom_warning_sent) {
	ntop->getTrace()->traceEvent(TRACE_WARNING, "Not enough memory");
	oom_warning_sent = true;
      }

      return;
    }

    l->run_script(path, NULL);
    delete l;
  } else
    ntop->getTrace()->traceEvent(TRACE_ERROR, "Missing script %s", path);
}

/* ******************************************* */

void PeriodicActivities::secondActivitiesLoop() {
  char script[MAX_PATH];

  snprintf(script, sizeof(script), "%s/%s", ntop->get_callbacks_dir(), SECOND_SCRIPT_PATH);

  while(!ntop->getGlobals()->isShutdown()) {
    runScript(script);
    sleep(1);
  }
}

/* ******************************************* */

void PeriodicActivities::minuteActivitiesLoop() {
  char script[MAX_PATH];

  snprintf(script, sizeof(script), "%s/%s", ntop->get_callbacks_dir(), MINUTE_SCRIPT_PATH);

  while(!ntop->getGlobals()->isShutdown()) {
    u_int now = (u_int)time(NULL);

    if((now % 60) == 0) runScript(script);
    sleep(1);
  }
}

/* ******************************************* */

void PeriodicActivities::hourActivitiesLoop() {
  char script[MAX_PATH];

  snprintf(script, sizeof(script), "%s/%s", ntop->get_callbacks_dir(), HOURLY_SCRIPT_PATH);

  while(!ntop->getGlobals()->isShutdown()) {
    u_int now = (u_int)time(NULL);

    if((now % 3600) == 0) runScript(script);
    sleep(1);
  }
}

/* ******************************************* */

void PeriodicActivities::dayActivitiesLoop() {
  char script[MAX_PATH];

  snprintf(script, sizeof(script), "%s/%s", ntop->get_callbacks_dir(), DAILY_SCRIPT_PATH);

  while(!ntop->getGlobals()->isShutdown()) {
    u_int now = (u_int)time(NULL);

    if((now % 86400) == 0) runScript(script);
    sleep(1);
  }
}
