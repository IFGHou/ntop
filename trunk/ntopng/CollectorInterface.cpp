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

#ifdef DARWIN
#include <uuid/uuid.h>
#endif

/* **************************************************** */

CollectorInterface::CollectorInterface(const char *_endpoint, const char *_script_name, bool change_user)
  : NetworkInterface(_endpoint, change_user) {

  endpoint = (char*)_endpoint, script_name = strdup(_script_name);

  l = new Lua();

  if(change_user) dropPrivileges();
}

/* **************************************************** */

CollectorInterface::~CollectorInterface() {
  shutdown();

  delete l;
  free(endpoint);
  free(script_name);

  deleteDataStructures();
}

/* **************************************************** */

void CollectorInterface::run_collector_script() {
  char script[256];

  snprintf(script, sizeof(script), "%s/%s", ntop->get_callbacks_dir(), script_name);

  ntop->getTrace()->traceEvent(TRACE_INFO, "Running flow collector %s.. [%s]", ifname, script);

  l->run_script(script);
}

/* **************************************************** */

static void* packetPollLoop(void* ptr) {
  CollectorInterface *iface = (CollectorInterface*)ptr;
  iface->run_collector_script();
  return(NULL);
}

/* **************************************************** */

void CollectorInterface::startPacketPolling() {
  pthread_create(&pollLoop, NULL, packetPollLoop, (void*)this);
  NetworkInterface::startPacketPolling();
}

/* **************************************************** */

void CollectorInterface::shutdown() {
  void *res;
  
  if(running) {
    NetworkInterface::shutdown();
    pthread_join(pollLoop, &res);
  }
}

/* **************************************************** */
