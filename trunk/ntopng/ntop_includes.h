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

#ifndef _NTOP_H_
#define _NTOP_H_

#include "config.h"

#include <stdio.h>
#include <stdarg.h>

#ifdef WIN32
#include "ntop_win32.h"
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <pthread.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/ethernet.h>
#include <netinet/ip.h>
#include <netinet/ip6.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <netinet/ip_icmp.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <syslog.h>
#include <netdb.h>
#include <dirent.h>
#include <pwd.h>
#endif


#ifdef linux
#define __FAVOR_BSD
#endif


#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <ctype.h>
#include <fcntl.h>
#include <getopt.h>
#include <string.h>
#include <sys/stat.h>
#include <zmq.h>

extern "C" {
#include "pcap.h"
#include "ndpi_main.h"
#include "luajit.h"
#include "lauxlib.h"
#include "lualib.h"
#ifdef HAVE_PF_RING
#include "pfring.h"
#endif
};

#include "third-party/json-parser/json.h"
#include "mongoose.h"
#include "ntop_defines.h"
#include "patricia.h"
#include "Trace.h"
#include "Utils.h"
#include "Redis.h"
#include "NtopGlobals.h"
#include "Prefs.h"
#include "Mutex.h"
#include "Serializable.h"
#include "IpAddress.h"
#include "NdpiStats.h"
#include "TrafficStats.h"
#include "ProtoStats.h"
#include "EthStats.h"
#include "GenericHashEntry.h"
#include "GenericHost.h"
#include "GenericHash.h"
#include "StringHost.h"
#include "StringHash.h"
#include "NetworkInterface.h"
#include "CollectorInterface.h"
#include "PcapInterface.h"
#ifdef HAVE_PF_RING
#include "PF_RINGInterface.h"
#endif
#include "Geolocation.h"
#include "GenericHost.h"
#include "Host.h"
#include "Flow.h"
#include "FlowHash.h"
#include "HostHash.h"
#include "PeriodicActivities.h"
#include "Lua.h"
#include "AddressResolution.h"
#include "Categorization.h"
#include "HTTPserver.h"
#include "Ntop.h"


#endif /* _NTOP_H_ */
