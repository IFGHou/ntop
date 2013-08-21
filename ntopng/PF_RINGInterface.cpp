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

#ifdef HAVE_PF_RING

#ifdef DARWIN
#include <uuid/uuid.h>
#endif

/* **************************************************** */

PF_RINGInterface::PF_RINGInterface(const char *name)
  : NetworkInterface(name) {

  if((pfring_handle = pfring_open(ifname, ntop->getGlobals()->getSnaplen(),
				  ntop->getGlobals()->getPromiscuousMode() ? PF_RING_PROMISC : 0)) == NULL) {
    throw 1;
  } else {
    u_int32_t version;

    pfring_version(pfring_handle, &version);
    ntop->getTrace()->traceEvent(TRACE_NORMAL, "Reading packets from PF_RING v.%d.%d.%d interface %s...",
				 (version & 0xFFFF0000) >> 16, (version & 0x0000FF00) >> 8, version & 0x000000FF,
				 ifname);
  }

  pcap_datalink_type = DLT_EN10MB;

  pfring_set_application_name(pfring_handle, (char*)"ntopng");
}

/* **************************************************** */

PF_RINGInterface::~PF_RINGInterface() {
  shutdown();

  if(pfring_handle)
    pfring_close(pfring_handle);

  deleteDataStructures();
}

/* **************************************************** */

static void pfring_packet_callback(const struct pfring_pkthdr *h, const u_char *p, const u_char *user_bytes) {
  NetworkInterface *iface = (NetworkInterface *) user_bytes;
  iface->packet_dissector((const struct pcap_pkthdr *) h, p);
}

/* **************************************************** */

static void* packetPollLoop(void* ptr) {
  PF_RINGInterface *iface = (PF_RINGInterface*)ptr;

  pfring_loop(iface->get_pfring_handle(), pfring_packet_callback, (u_char*) iface, 1 /* wait mode */);

  return(NULL);
}

/* **************************************************** */

void PF_RINGInterface::startPacketPolling() {
  pthread_create(&pollLoop, NULL, packetPollLoop, (void*)this);
  NetworkInterface::startPacketPolling();
}

/* **************************************************** */

void PF_RINGInterface::shutdown() {
  void *res;

  if(running) { 
    if(pfring_handle) pfring_breakloop(pfring_handle);
    pthread_join(pollLoop, &res);
    NetworkInterface::shutdown();
  }
}

/* **************************************************** */

u_int PF_RINGInterface::getNumDroppedPackets() {
  pfring_stat stats;
 
  if(pfring_stats(pfring_handle, &stats) >= 0) {
#if 0
    ntop->getTrace()->traceEvent(TRACE_NORMAL, "[%s][Rcvd: %llu][Drops: %llu][DroppedByFilter: %u]", 
				 ifname, stats.recv, stats.drop, stats.droppedbyfilter);
#endif
    return(stats.drop);
  }

  return(0);
}

/* **************************************************** */

bool PF_RINGInterface::set_packet_filter(char *filter) {
  if(pfring_set_bpf_filter(pfring_handle, filter) != 0) {
    ntop->getTrace()->traceEvent(TRACE_ERROR, "Unable to set filter %s. Filter ignored.\n", filter);
    return(false);
  } else {
    ntop->getTrace()->traceEvent(TRACE_NORMAL, "Packet capture filter set to \"%s\"", filter);
    return(true);
  }
}

/* **************************************************** */

#endif /* HAVE_PF_RING */
