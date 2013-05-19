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
  ntop->getTrace()->traceEvent(TRACE_NORMAL, "Started periodic activities loop...");

  pthread_create(&secondLoop, NULL, secondStartLoop, (void*)this);
  pthread_create(&minuteLoop, NULL, minuteStartLoop, (void*)this);
  pthread_create(&hourLoop,   NULL, hourStartLoop,   (void*)this);
  pthread_create(&dayLoop,    NULL, dayStartLoop,    (void*)this);
}

/* ******************************************* */

void PeriodicActivities::runScript(char *path) {
  struct stat statbuf;

  if(stat(path, &statbuf) == 0) {
    Lua *l = new Lua();

    ntop->getTrace()->traceEvent(TRACE_INFO, "Running script %s", path);

    l->run_script(path);
    delete l;
  } else
    ntop->getTrace()->traceEvent(TRACE_ERROR, "Missing script %s", path);
}

/* ******************************************* */

void PeriodicActivities::secondActivitiesLoop() {
  char script[256];

  snprintf(script, sizeof(script), "%s/%s", ntop->get_callbacks_dir(), SECOND_SCRIPT_PATH);

  while(!ntop->getGlobals()->isShutdown()) {
    runScript(script);
    sleep(1);
  }
}

/* ******************************************* */

void PeriodicActivities::minuteActivitiesLoop() {
  char script[256];

  snprintf(script, sizeof(script), "%s/%s", ntop->get_callbacks_dir(), MINUTE_SCRIPT_PATH);

  while(!ntop->getGlobals()->isShutdown()) {
    u_int now = (u_int)time(NULL);

    if((now % 60) == 0) runScript(script);
    sleep(1);
  }
}

/* ******************************************* */

void PeriodicActivities::hourActivitiesLoop() {
  char script[256];

  snprintf(script, sizeof(script), "%s/%s", ntop->get_callbacks_dir(), HOURLY_SCRIPT_PATH);

  while(!ntop->getGlobals()->isShutdown()) {
    u_int now = (u_int)time(NULL);

    if((now % 3600) == 0) runScript(script);
    sleep(1);
  }
}

/* ******************************************* */

void PeriodicActivities::dayActivitiesLoop() {
  char script[256];

  snprintf(script, sizeof(script), "%s/%s", ntop->get_callbacks_dir(), DAILY_SCRIPT_PATH);

  while(!ntop->getGlobals()->isShutdown()) {
    u_int now = (u_int)time(NULL);

    if((now % 86400) == 0) runScript(script);
    sleep(1);
  }
}
