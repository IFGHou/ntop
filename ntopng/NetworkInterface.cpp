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

#ifndef ETH_P_IP
#define ETH_P_IP 0x0800
#endif

#define GTP_U_V1_PORT      2152

/* **************************************************** */

NetworkInterface::NetworkInterface(char *name) {
  char pcap_error_buffer[PCAP_ERRBUF_SIZE];

  ifname = strdup(name);
  ifStats = new InterfaceStats();

  if((pcap_handle = pcap_open_live(ifname, ntop->getGlobals()->getSnaplen(),
				   ntop->getGlobals()->getPromiscuousMode(),
				   500, pcap_error_buffer)) == NULL) {
    pcap_handle = pcap_open_offline(ifname, pcap_error_buffer);

    if(pcap_handle == NULL) {
      printf("ERROR: could not open pcap file: %s\n", pcap_error_buffer);
      throw "Unable to open network interface ";
    } else
      ntop->getTrace()->traceEvent(TRACE_NORMAL, "Reading packets from pcap file %s...", ifname);
  } else
    ntop->getTrace()->traceEvent(TRACE_NORMAL, "Reading packets from interface %s...", ifname);

  pcap_datalink_type = pcap_datalink(pcap_handle);

  memset(ndpi_flows_root, 0, sizeof(ndpi_flows_root));
  ndpi_flow_count = 0;
}

/* **************************************************** */

NetworkInterface::~NetworkInterface() {
  if(pcap_handle)
    pcap_close(pcap_handle);

  free(ifname);
  delete ifStats;
}

/* **************************************************** */

static int node_cmp(const void *a, const void *b) {
  Flow *fa = (Flow*)a;
  Flow *fb = (Flow*)b;

  return(fa->compare(fb));
}

/* **************************************************** */

static void node_proto_guess_walker(const void *node, ndpi_VISIT which, int depth) {
  /* Avoid walking the same node multiple times */
  if((which == ndpi_preorder) || (which == ndpi_leaf)) {
    struct Flow *flow = *(Flow**)node;

    flow->print();  
  }
}

/* **************************************************** */

void NetworkInterface::dumpFlows() {
  for (int i=0; i<NUM_ROOTS; i++) {
    ndpi_twalk(ndpi_flows_root[i], node_proto_guess_walker);
  }

}

/* **************************************************** */

Flow* NetworkInterface::getFlow(u_int16_t vlan_id, const struct ndpi_iphdr *iph, u_int16_t ipsize) {
  u_int32_t idx;
  u_int16_t l4_packet_len;
  struct ndpi_tcphdr *tcph = NULL;
  struct ndpi_udphdr *udph = NULL;
  u_int32_t lower_ip;
  u_int32_t upper_ip;
  u_int16_t lower_port;
  u_int16_t upper_port;
  void *ret;
  Flow *flowKey;

  if(ipsize < 20)
    return NULL;

  if((iph->ihl * 4) > ipsize || ipsize < ntohs(iph->tot_len)
     || (iph->frag_off & htons(0x1FFF)) != 0)
    return NULL;

  l4_packet_len = ntohs(iph->tot_len) - (iph->ihl * 4);

  if(iph->saddr < iph->daddr) {
    lower_ip = iph->saddr;
    upper_ip = iph->daddr;
  } else {
    lower_ip = iph->daddr;
    upper_ip = iph->saddr;
  }

  if(iph->protocol == 6 && l4_packet_len >= 20) {
    // tcp
    tcph = (struct ndpi_tcphdr *) ((u_int8_t *) iph + iph->ihl * 4);
    if(iph->saddr < iph->daddr) {
      lower_port = tcph->source;
      upper_port = tcph->dest;
    } else {
      lower_port = tcph->dest;
      upper_port = tcph->source;
    }
  } else if(iph->protocol == 17 && l4_packet_len >= 8) {
    // udp
    udph = (struct ndpi_udphdr *) ((u_int8_t *) iph + iph->ihl * 4);
    if(iph->saddr < iph->daddr) {
      lower_port = udph->source;
      upper_port = udph->dest;
    } else {
      lower_port = udph->dest;
      upper_port = udph->source;
    }
  } else {
    // non tcp/udp protocols
    lower_port = 0;
    upper_port = 0;
  }

  flowKey = new Flow(vlan_id, iph->protocol, lower_ip, lower_port, upper_ip, upper_port);
  idx = (vlan_id + lower_ip + upper_ip + iph->protocol + lower_port + upper_port) % NUM_ROOTS;
  ret = (void*)ndpi_tfind(flowKey, (void*)&ndpi_flows_root[idx], node_cmp);

  if(ret == NULL) {
#if TODO
    if(ndpi_flow_count == MAX_NDPI_FLOWS) {
      printf("ERROR: maximum flow count (%u) has been exceeded\n", MAX_NDPI_FLOWS);
      return(NULL);
    } else
#endif
      {
      flowKey->allocFlowMemory();
      ndpi_tsearch(flowKey, (void**)&ndpi_flows_root[idx], node_cmp); /* Add */
      return(flowKey);
    }
  } else {
    delete flowKey;
    return(*(Flow**)ret);
  }
}

/* **************************************************** */

void NetworkInterface::packet_processing(const u_int64_t time, u_int16_t vlan_id,
					 const struct ndpi_iphdr *iph,
					 u_int16_t ipsize, u_int16_t rawsize)
{
  Flow *flow = getFlow(vlan_id, iph, ipsize);
  u_int16_t frag_off = ntohs(iph->frag_off);

  if(flow == NULL) return; else flow->incStats(rawsize);
  if(flow->isDetectionCompleted()) return;

  // only handle unfragmented packets
  if((frag_off & 0x3FFF) == 0) {
    struct ndpi_flow_struct *ndpi_flow = flow->get_ndpi_flow();
    struct ndpi_id_struct *src = (struct ndpi_id_struct*)flow->get_src_id();
    struct ndpi_id_struct *dst = (struct ndpi_id_struct*)flow->get_dst_id();

    flow->setDetectedProtocol(ndpi_detection_process_packet(ntop->getGlobals()->get_ndpi_struct(),
							    ndpi_flow, (uint8_t *)iph, ipsize, time, src, dst),
			      iph->protocol);
  }
}

/* **************************************************** */

static void pcap_packet_callback(u_char * args, const struct pcap_pkthdr *header, const u_char * packet) {
  NetworkInterface *iface = (NetworkInterface*)args;
  const struct ndpi_ethhdr *ethernet;
  struct ndpi_iphdr *iph;
  u_int64_t time;
  static u_int64_t lasttime = 0;
  u_int16_t type, ip_offset, vlan_id = 0;
  u_int32_t res = ntop->getGlobals()->get_detection_tick_resolution();
  int pcap_datalink_type = iface->get_datalink();

  iface->incStats(header->caplen);

  time = ((uint64_t) header->ts.tv_sec) * res + header->ts.tv_usec / (1000000 / res);
  if(lasttime > time) time = lasttime;

  lasttime = time;

  if(pcap_datalink_type == DLT_EN10MB) {
    ethernet = (struct ndpi_ethhdr *) packet;
    ip_offset = sizeof(struct ndpi_ethhdr);
    type = ntohs(ethernet->h_proto);
  } else if(pcap_datalink_type == 113 /* Linux Cooked Capture */) {
    type = (packet[14] << 8) + packet[15];
    ip_offset = 16;
  } else
    return;

  if(type == 0x8100 /* VLAN */) {
    Ether80211q *qType = (Ether80211q*)&packet[ip_offset];
    vlan_id = ntohs(qType->vlanId) & 0xFFF;

    type = (packet[ip_offset+2] << 8) + packet[ip_offset+3];
    ip_offset += 4;
  }

  iph = (struct ndpi_iphdr *) &packet[ip_offset];

  // just work on Ethernet packets that contain IP
  if(type == ETH_P_IP && header->caplen >= ip_offset) {
    u_int16_t frag_off = ntohs(iph->frag_off);

    if(iph->version != 4) {
      /* FIX - Add IPv6 support */
      return;
    }

    if(ntop->getGlobals()->decode_tunnels() && (iph->protocol == IPPROTO_UDP) && ((frag_off & 0x3FFF) == 0)) {
      u_short ip_len = ((u_short)iph->ihl * 4);
      struct ndpi_udphdr *udp = (struct ndpi_udphdr *)&packet[ip_offset+ip_len];
      u_int16_t sport = ntohs(udp->source), dport = ntohs(udp->dest);

      if((sport == GTP_U_V1_PORT) || (dport == GTP_U_V1_PORT)) {
	/* Check if it's GTPv1 */
	u_int offset = (u_int)(ip_offset+ip_len+sizeof(struct ndpi_udphdr));
	u_int8_t flags = packet[offset];
	u_int8_t message_type = packet[offset+1];

	if((((flags & 0xE0) >> 5) == 1 /* GTPv1 */) && (message_type == 0xFF /* T-PDU */)) {
	  ip_offset = ip_offset+ip_len+sizeof(struct ndpi_udphdr)+8 /* GTPv1 header len */;

	  if(flags & 0x04) ip_offset += 1; /* next_ext_header is present */
	  if(flags & 0x02) ip_offset += 4; /* sequence_number is present (it also includes next_ext_header and pdu_number) */
	  if(flags & 0x01) ip_offset += 1; /* pdu_number is present */

	  iph = (struct ndpi_iphdr *) &packet[ip_offset];

	  if(iph->version != 4) {
	    /* FIX - Add IPv6 support */
	    return;
	  }
	}
      }

    }

    iface->packet_processing(time, vlan_id, iph, header->len - ip_offset, header->len);
  }
}

/* **************************************************** */

static void* packetPollLoop(void* ptr) {
  NetworkInterface *iface = (NetworkInterface*)ptr;

  pcap_loop(iface->get_pcap_handle(), -1, &pcap_packet_callback, (u_char*)iface);
  return(NULL);
}


/* **************************************************** */

void NetworkInterface::startPacketPolling() {
  ntop->getTrace()->traceEvent(TRACE_NORMAL, "Started packet polling...");

  pthread_create(&pollLoop, NULL, packetPollLoop, (void*)this);

}

/* **************************************************** */

void NetworkInterface::shutdown() {
  pcap_breakloop(pcap_handle);
}

/* **************************************************** */

