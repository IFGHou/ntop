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

#ifndef _PREFS_H_
#define _PREFS_H_

#include "ntop_includes.h"
#include "credis.h"

class Prefs {
  REDIS redis;
  Mutex *l;

  void setDefaults();

 public:
  Prefs(char *redis_host = (char*)"127.0.0.1", int redis_port = 6379);
  ~Prefs();

  int get(char *key, char *rsp, u_int rsp_len);
  int set(char *key, char *value, u_int expire_secs=0);
  int queueHostToResolve(char *hostname);
  int popHostToResolve(char *hostname, u_int hostname_len);
};

#endif /* _PREFS_H_ */
