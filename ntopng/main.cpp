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

#include "ntop.h"

NetworkInterface *iface = NULL;
HTTPserver *httpd = NULL;

/* ******************************************* */

static void help() {
  printf("ntopng - (C) 1998-13 ntop.org\n\n"
	 "-i <interface>       | Input interface name\n"
	 "-w <http port>       | HTTP port\n"
	 );
  exit(0);
}


/* ******************************** */

void sigproc(int sig) {
  static int called = 0;

  ntopGlobals->getTrace()->traceEvent(TRACE_NORMAL, "Leaving...");
  if(called) return; else called = 1;
  ntopGlobals->doShutdown();  

  if(iface) {
    InterfaceStats *stats = iface->getStats();
    
    stats->printStats();
    iface->shutdown();
  }

  if(httpd)
    delete httpd;

  delete ntopGlobals;
  exit(0);
}

/* ******************************************* */

int main(int argc, char *argv[]) {
  u_char c;
  char *ifName = NULL;
  u_int http_port = 3000;

  ntopGlobals = new NtopGlobals();

  while((c = getopt(argc, argv, "hi:w:")) != '?') {
    if(c == 255) break;

    switch(c) {
    case 'h':
      help();
    case 'i':
      ifName = strdup(optarg);
      break;
    case 'w':
      http_port = atoi(optarg);
      break;
    }
  }

  if(ifName == NULL)
    help();
  
  iface = new NetworkInterface(ifName);
  httpd = new HTTPserver(http_port, "./httpdocs", "./scripts");

  signal(SIGINT, sigproc);
  signal(SIGTERM, sigproc);
  signal(SIGINT, sigproc);

  iface->startPacketPolling();
  iface->dumpFlows();

  while(1) sleep(1);
  sigproc(0);
  return(0);
}
