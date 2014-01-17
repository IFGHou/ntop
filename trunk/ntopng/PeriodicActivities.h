/*
 *
 * (C) 2013-14 - ntop.org
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

#ifndef _PERIODIC_ACTIVITIES_H_
#define _PERIODIC_ACTIVITIES_H_

#include "ntop_includes.h"

#define SECOND_SCRIPT_PATH   "second.lua"
#define MINUTE_SCRIPT_PATH   "minute.lua"
#define HOURLY_SCRIPT_PATH   "hourly.lua"
#define DAILY_SCRIPT_PATH    "daily.lua"

class PeriodicActivities {
 private:
  pthread_t secondLoop, minuteLoop, hourLoop, dayLoop;

  void runScript(char *path);

 public:
  PeriodicActivities();
  ~PeriodicActivities();

  void loop();
  void startPeriodicActivitiesLoop();
  void secondActivitiesLoop();
  void minuteActivitiesLoop();
  void hourActivitiesLoop();
  void dayActivitiesLoop();
};

#endif /* _PERIODIC_ACTIVITIES_H_ */
