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

#include "ntop_includes.h"

/* *************************************** */

Host::Host(NetworkInterface *_iface) : GenericHost(_iface) {
  ip = new IpAddress();
  initialize(NULL, 0, false);
}

/* *************************************** */

Host::Host(NetworkInterface *_iface, char *ipAddress) : GenericHost(_iface) {
  ip = new IpAddress(ipAddress);
  initialize(NULL, 0, false);
}

/* *************************************** */

Host::Host(NetworkInterface *_iface, u_int8_t mac[6],
	   u_int16_t _vlanId, IpAddress *_ip) : GenericHost(_iface) {
  ip = new IpAddress(_ip);
  initialize(mac, _vlanId, true);
}

/* *************************************** */

Host::Host(NetworkInterface *_iface, u_int8_t mac[6],
	   u_int16_t _vlanId) : GenericHost(_iface) {
  ip = NULL;
  initialize(mac, _vlanId, true);
}

/* *************************************** */

void Host::read_alternate_name() {  
  char buf[64], rsp[64], *host = ip->print(buf, sizeof(buf));
  
  if(ntop->getRedis()->hashGet((char*)HOST_ALTERNATE_NAME, host, rsp, sizeof(rsp)) == 0) {
    if(alternate_name) free(alternate_name);
    alternate_name = strdup(rsp);  
  }
}

/* *************************************** */

void Host::save_alternate_name() {
  if(alternate_name) {
    char buf[64], *host = ip->print(buf, sizeof(buf));

    ntop->getRedis()->hashSet((char*)HOST_ALTERNATE_NAME, host, alternate_name);
  }
}

/* *************************************** */

Host::~Host() {
  char key[128], *k;

  if(num_uses > 0)
    ntop->getTrace()->traceEvent(TRACE_WARNING, "Internal error: num_uses=%u", num_uses);

  if(ip && (!ip->isEmpty())) dumpStats(false);

  k = get_string_key(key, sizeof(key));

  /* Host Contacts */
  if(k && (k[0] != '\0')) {
    char key[64];

    snprintf(key, sizeof(key), "%s.client", k);
    ntop->getRedis()->del(key);

    snprintf(key, sizeof(key), "%s.server", k);
    ntop->getRedis()->del(key);
  }

  if(localHost || systemHost) {
    if(ip != NULL) {
      snprintf(key, sizeof(key), "%s.client", k);
      ntop->getRedis()->del(key);

      snprintf(key, sizeof(key), "%s.server", k);
      ntop->getRedis()->del(key);
    }
  }

  dumpHostContacts(HOST_FAMILY_ID);

  save_alternate_name();

  // ntop->getTrace()->traceEvent(TRACE_NORMAL, "Deleting %s (%s)", k, localHost ? "local": "remote");

  if((localHost || systemHost)
     && ntop->getPrefs()->is_host_persistency_enabled()) {
    char *json = serialize();
    snprintf(key, sizeof(key), "%s.%d.json", k, vlan_id);
    ntop->getRedis()->set(key, json, 3600 /* 1 hour */);
    //ntop->getTrace()->traceEvent(TRACE_NORMAL, "%s => %s", k, json);
    free(json);
  }

  if(dns)            delete dns;
  if(epp)            delete epp;

  if(symbolic_name)  free(symbolic_name);
  if(alternate_name) free(alternate_name);
  if(country)        free(country);
  if(city)           free(city);
  if(asname)         free(asname);

  delete syn_flood_alert;
  delete ip;
  delete m;
}

/* *************************************** */

void Host::computeHostSerial() {
  if(ip
     && iface
     && Utils::dumpHostToDB(ip, ntop->getPrefs()->get_dump_hosts_to_db_policy())) {
    if(host_serial) {
      char buf[64];

      /* We need to reconfirm the id (e.g. after a day wrap) */
      ntop->getRedis()->setHostId(iface, NULL, ip->print(buf, sizeof(buf)), host_serial);
    } else
      host_serial = ntop->getRedis()->addHostToDBDump(iface, ip, NULL);
  }
}

/* *************************************** */

void Host::flushContacts(bool freeHost) {
  dumpHostContacts(HOST_FAMILY_ID);
  contacts->purgeAll();

  if(!freeHost) {
    /*
      Recompute it so that if the day wrapped
      we have a new one
    */
    computeHostSerial();
  }
}

/* *************************************** */

void Host::initialize(u_int8_t mac[6], u_int16_t _vlanId, bool init_all) {
  char key[64], redis_key[128], json[4096], *k;

  if(mac) memcpy(mac_address, mac, 6); else memset(mac_address, 0, 6);

  //if(_vlanId > 0) ntop->getTrace()->traceEvent(TRACE_NORMAL, "VLAN => %d", _vlanId);

  syn_flood_alert = new AlertCounter(ntop->getPrefs()->get_host_max_new_syn_sec(), CONST_MAX_THRESHOLD_CROSS_DURATION);
  category[0] = '\0', os[0] = '\0', httpbl[0] = '\0', blacklisted_host = false;
  num_uses = 0, symbolic_name = alternate_name = NULL, vlan_id = _vlanId;
  first_seen = last_seen = iface->getTimeLastPktRcvd();
  if((m = new(std::nothrow) Mutex()) == NULL)
    ntop->getTrace()->traceEvent(TRACE_WARNING, "Internal error: NULL mutex. Are you running out of memory?");

  asn = 0, asname = NULL, country = NULL, city = NULL;
  longitude = 0, latitude = 0;
  k = get_string_key(key, sizeof(key));
  snprintf(redis_key, sizeof(redis_key), "%s.%d.json", k, vlan_id);
  dns = NULL, epp = NULL;

  if(init_all) {
    if(ip) {
      char buf[64], rsp[256], *host = ip->print(buf, sizeof(buf));

      updateLocal();
      systemHost = ip->isLocalInterfaceAddress();

      // ntop->getTrace()->traceEvent(TRACE_NORMAL, "Loading %s (%s)", k, localHost ? "local": "remote");

      if(((localHost || systemHost)
	  && ntop->getPrefs()->is_host_persistency_enabled())
	 && (!ntop->getRedis()->get(redis_key, json, sizeof(json)))) {
	/* Found saved copy of the host so let's start from the previous state */
	// ntop->getTrace()->traceEvent(TRACE_NORMAL, "%s => %s", redis_key, json);
	deserialize(json);
      }

      if(localHost || systemHost
	 || ntop->getPrefs()->is_dns_resolution_enabled_for_all_hosts()) {
	if(ntop->getRedis()->getAddress(host, rsp, sizeof(rsp), true) == 0) {
	  if(symbolic_name) free(symbolic_name);
	  symbolic_name = strdup(rsp);
	}
	// else ntop->getRedis()->pushHostToResolve(host, false, localHost);
      }

      if((!localHost || systemHost) && ntop->getPrefs()->is_httpbl_enabled() && ip->isIPv4()) {
	// http:bl only works for IPv4 addresses
	if(ntop->getRedis()->getAddressHTTPBL(host, iface, httpbl, sizeof(httpbl), true) == 0) {
          if(strcmp(httpbl, NULL_BL)) {
	    blacklisted_host = true;
	    ntop->getTrace()->traceEvent(TRACE_INFO,"Blacklisted host found %s [%s]", host, httpbl);
	  }
	}
      }

      if(asname) { free(asname); asname = NULL; }
      ntop->getGeolocation()->getAS(ip, &asn, &asname);

      if(country) { free(country); country = NULL; }
      if(city)    { free(city); city = NULL; }
      ntop->getGeolocation()->getInfo(ip, &country, &city, &latitude, &longitude);
      read_alternate_name();
    } else {
      char buf[32];

      snprintf(buf, sizeof(buf), "%02X:%02X:%02X:%02X:%02X:%02X",
	       mac_address[0], mac_address[1], mac_address[2],
	       mac_address[3], mac_address[4], mac_address[5]);

      if(symbolic_name) free(symbolic_name);
      symbolic_name = strdup(buf);
      localHost = true;
    }

    if((localHost || systemHost)
       && ip) readStats();
  }

  if(!host_serial) computeHostSerial();
}

/* *************************************** */

void Host::updateLocal() {
  localHost = ip->isLocalHost(&local_network_id);

  if(0) {
    char buf[64];

    ntop->getTrace()->traceEvent(TRACE_NORMAL, "%s is %s",
				 ip->print(buf, sizeof(buf)), localHost ? "local" : "remote");
  }
}

/* *************************************** */

char* Host::get_mac(char *buf, u_int buf_len) {
  snprintf(buf, buf_len,
	   "%02X:%02X:%02X:%02X:%02X:%02X",
	   mac_address[0] & 0xFF, mac_address[1] & 0xFF,
	   mac_address[2] & 0xFF, mac_address[3] & 0xFF,
	   mac_address[4] & 0xFF, mac_address[5] & 0xFF);

  return(buf);
}

/* *************************************** */

void Host::set_mac(char *m) {
  u_int32_t mac[6] = { 0 };

  sscanf(m, "%u:%u:%u:%u:%u:%u",
         &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]);

  mac_address[0] = mac[0], mac_address[1] = mac[1],
	  mac_address[2] = mac[2], mac_address[3] = mac[3],
	  mac_address[4] = mac[4], mac_address[5] = mac[5];
}

/* *************************************** */

void Host::lua(lua_State* vm, bool host_details, bool verbose, bool returnHost) {
  char buf[64];
  char buf_id[64];

  if(host_details) {
    char *ipaddr = NULL, *local_net;

    lua_newtable(vm);

    lua_push_bool_table_entry(vm, "privatehost", isPrivateHost());
    lua_push_str_table_entry(vm, "mac", get_mac(buf, sizeof(buf)));
    
    if(ip)
      lua_push_str_table_entry(vm, "ip", (ipaddr = ip->print(buf, sizeof(buf))));
    else
      lua_push_nil_table_entry(vm, "ip");

    if((ipaddr != NULL)
       && ((symbolic_name == NULL) || (strcmp(symbolic_name, ipaddr) == 0))) {
      /* We resolve immediately the IP address by queueing on the top of address queue */
      
      ntop->getRedis()->pushHostToResolve(ipaddr, false, true /* Fake to resolve it ASAP */);
      lua_push_str_table_entry(vm, "name", get_name(buf, sizeof(buf), false));
    }
    
    lua_push_str_table_entry(vm, "alternate_name", alternate_name ? alternate_name : (char*)"");

    lua_push_int_table_entry(vm, "vlan", vlan_id);
    lua_push_bool_table_entry(vm, "localhost", localHost);
    lua_push_int32_table_entry(vm, "local_network_id", local_network_id);

    local_net = ntop->getLocalNetworkName(local_network_id);
    if(local_net == NULL)
      lua_push_nil_table_entry(vm, "local_network_name");
    else
      lua_push_str_table_entry(vm, "local_network_name", local_net);
    lua_push_bool_table_entry(vm, "systemhost", systemHost);
    lua_push_int_table_entry(vm, "source_id", source_id);
    lua_push_int_table_entry(vm, "asn", ip ? asn : 0);

    lua_push_str_table_entry(vm, "asname", ip ? asname : (char*)"");
    lua_push_str_table_entry(vm, "os", os);

    lua_push_float_table_entry(vm, "latitude", latitude);
    lua_push_float_table_entry(vm, "longitude", longitude);
    lua_push_str_table_entry(vm, "city", city ? city : (char*)"");

    lua_push_str_table_entry(vm, "country", country ? country : (char*)"");
    lua_push_int_table_entry(vm, "bytes.sent", sent.getNumBytes());
    lua_push_int_table_entry(vm, "bytes.rcvd", rcvd.getNumBytes());
    lua_push_int_table_entry(vm, "pkts.sent", sent.getNumPkts());
    lua_push_int_table_entry(vm, "pkts.rcvd", rcvd.getNumPkts());
    lua_push_int_table_entry(vm, "flows.as_client", num_flows_as_client);
    lua_push_int_table_entry(vm, "flows.as_server", num_flows_as_server);

    if(ip) {
      lua_push_int_table_entry(vm, "udp.pkts.sent",  udp_sent.getNumPkts());
      lua_push_int_table_entry(vm, "udp.bytes.sent", udp_sent.getNumBytes());
      lua_push_int_table_entry(vm, "udp.pkts.rcvd",  udp_rcvd.getNumPkts());
      lua_push_int_table_entry(vm, "udp.bytes.rcvd", udp_rcvd.getNumBytes());

      lua_push_int_table_entry(vm, "tcp.pkts.sent",  tcp_sent.getNumPkts());
      lua_push_int_table_entry(vm, "tcp.bytes.sent", tcp_sent.getNumBytes());
      lua_push_int_table_entry(vm, "tcp.pkts.rcvd",  tcp_rcvd.getNumPkts());
      lua_push_int_table_entry(vm, "tcp.bytes.rcvd", tcp_rcvd.getNumBytes());

      lua_push_int_table_entry(vm, "icmp.pkts.sent",  icmp_sent.getNumPkts());
      lua_push_int_table_entry(vm, "icmp.bytes.sent", icmp_sent.getNumBytes());
      lua_push_int_table_entry(vm, "icmp.pkts.rcvd",  icmp_rcvd.getNumPkts());
      lua_push_int_table_entry(vm, "icmp.bytes.rcvd", icmp_rcvd.getNumBytes());

      lua_push_int_table_entry(vm, "other_ip.pkts.sent",  other_ip_sent.getNumPkts());
      lua_push_int_table_entry(vm, "other_ip.bytes.sent", other_ip_sent.getNumBytes());
      lua_push_int_table_entry(vm, "other_ip.pkts.rcvd",  other_ip_rcvd.getNumPkts());
      lua_push_int_table_entry(vm, "other_ip.bytes.rcvd", other_ip_rcvd.getNumBytes());
    }

    lua_push_int_table_entry(vm, "seen.first", first_seen);
    lua_push_int_table_entry(vm, "seen.last", last_seen);
    lua_push_int_table_entry(vm, "duration", get_duration());

    lua_push_float_table_entry(vm, "throughput_bps", bytes_thpt);
    lua_push_int_table_entry(vm, "throughput_trend_bps", bytes_thpt_trend);
    // ntop->getTrace()->traceEvent(TRACE_NORMAL, "[bytes_thpt: %.2f] [bytes_thpt_trend: %d]", bytes_thpt,bytes_thpt_trend);
    lua_push_float_table_entry(vm, "throughput_pps", pkts_thpt);
    lua_push_int_table_entry(vm, "throughput_trend_pps", pkts_thpt_trend);
    // ntop->getTrace()->traceEvent(TRACE_NORMAL, "[pkts_thpt: %.2f] [pkts_thpt_trend: %d]", pkts_thpt,pkts_thpt_trend);
    lua_push_int_table_entry(vm, "num_alerts", getNumAlerts());

    if(ip) {
      if(ntop->get_categorization()) lua_push_str_table_entry(vm, "category", get_category());
      if(ntop->getPrefs()->is_httpbl_enabled()) lua_push_str_table_entry(vm, "httpbl", get_httpbl());
    }

    if(verbose) {
      char *rsp = serialize();

      if(ndpiStats) ndpiStats->lua(iface, vm);
      getHostContacts(vm);
      lua_push_str_table_entry(vm, "json", rsp);
      free(rsp);

      sent_stats.lua(vm, "pktStats.sent");
      recv_stats.lua(vm, "pktStats.recv");

      if(dns) dns->lua(vm);
      if(epp) epp->lua(vm);
    }

    if(!returnHost) {
      /*Use the ip@vlan_id as a key only in case of multi vlan_id, otherwise use only the ip as a key*/
      if (vlan_id == 0) {
        sprintf(buf_id, "%s",(ip != NULL) ? ip->print(buf, sizeof(buf)) : get_mac(buf, sizeof(buf)));
      } else {
        sprintf(buf_id, "%s@%d",(ip != NULL) ? ip->print(buf, sizeof(buf)) : get_mac(buf, sizeof(buf)),vlan_id );
      }
      
      lua_pushstring(vm,buf_id);
      lua_insert(vm, -2);
      lua_settable(vm, -3);
    }
  } else {
    lua_newtable(vm);
     char *ipaddr = NULL;
    if(ip)
      lua_push_str_table_entry(vm, "ip", (ipaddr = ip->print(buf, sizeof(buf))));
    else
      lua_push_nil_table_entry(vm, "ip");
    lua_push_str_table_entry(vm, "mac", get_mac(buf, sizeof(buf)));
    lua_push_int_table_entry(vm, "vlan", vlan_id);
    lua_push_int_table_entry(vm, "traffic",  (lua_Integer)(sent.getNumBytes()+rcvd.getNumBytes()));
    lua_push_bool_table_entry(vm, "localHost", localHost);

    /*Use the ip@vlan_id as a key only in case of multi vlan_id, otherwise use only the ip as a key*/
    if (vlan_id == 0) {
      sprintf(buf_id, "%s",(ip != NULL) ? ip->print(buf, sizeof(buf)) : get_mac(buf, sizeof(buf)));
    } else {
      sprintf(buf_id, "%s@%d",(ip != NULL) ? ip->print(buf, sizeof(buf)) : get_mac(buf, sizeof(buf)),vlan_id );
    }
   
    lua_pushstring(vm,buf_id);
    lua_insert(vm, -2);
    lua_settable(vm, -3);
  }
}

/* ***************************************** */

/*
  As this method can be called from Lua, in order to avoid concurency issues
  we need to lock/unlock
*/
void Host::setName(char *name, bool update_categorization) {
  bool to_categorize = false;

  if(m) m->lock(__FILE__, __LINE__);
  if((symbolic_name == NULL) || (symbolic_name && strcmp(symbolic_name, name))) {
    if(symbolic_name) free(symbolic_name);
    symbolic_name = strdup(name);
    to_categorize = true;
  }
  if(m) m->unlock(__FILE__, __LINE__);

  if(to_categorize && ntop->get_categorization())
    ntop->get_categorization()->findCategory(symbolic_name, category, sizeof(category),
					     update_categorization);
}

/* ***************************************** */

void Host::set_alternate_name(char *name) {
  if(alternate_name) {
    if(!strcmp(name, alternate_name))
      return; /* Nothing changed */
    else {
      free(alternate_name);
      alternate_name = NULL;
    }
  }

  alternate_name = strdup(name);
  save_alternate_name();
}

/* ***************************************** */

void Host::refreshCategory() {
  if((symbolic_name != NULL) && (category[0] == '\0') && ntop->get_categorization()) {
    ntop->get_categorization()->findCategory(symbolic_name, category, sizeof(category), false);
  }
}

/* ***************************************** */

void Host::refreshHTTPBL() {
  if(ip && ip->isIPv4() && (!localHost) && (httpbl[0] == '\0') && ntop->getPrefs()->is_httpbl_enabled()) {
    char buf[128] =  { 0 };
    char* ip_addr = ip->print(buf, sizeof(buf));

    ntop->get_httpbl()->findHTTPBL(ip_addr, httpbl, sizeof(httpbl), false);
  }
}

/* ***************************************** */

char* Host::get_name(char *buf, u_int buf_len, bool force_resolution_if_not_found) {
  if(ip == NULL) {
    return(get_mac(buf, buf_len));
  } else {
    char *addr, redis_buf[64];
    int rc;

    addr = ip->print(buf, buf_len);

    if((symbolic_name != NULL) && strcmp(symbolic_name, addr))
      return(symbolic_name);

    rc = ntop->getRedis()->getAddress(addr, redis_buf, sizeof(redis_buf),
				      force_resolution_if_not_found);

    if(rc == 0)
      setName(redis_buf, false);
    else {
      if(symbolic_name != NULL) free(symbolic_name);

      symbolic_name = strdup(addr);
    }

    return(symbolic_name);
  }
}

/* *************************************** */

int Host::compare(Host *h) {
  if(ip)
    return(ip->compare(h->ip));
  else
    return(memcmp(mac_address, h->mac_address, 6));
}

/* ***************************************** */

bool Host::idle() {
  if(num_uses > 0) return(false);
  
  if(!iface->is_purge_idle_interface()) return(false);

  switch(ntop->getPrefs()->get_host_stickness()) {
  case location_none:
    break;

  case location_local_only:
    if(localHost || systemHost) return(false);
    break;

  case location_remote_only:
    if(!(localHost||systemHost)) return(false);
    break;

  case location_all:
    return(false);
    break;
  }

  return(isIdle(ntop->getPrefs()->get_host_max_idle(localHost)));
};

/* ***************************************** */

u_int32_t Host::key() {
  if(ip)
    return(ip->key());
  else {
    u_int32_t hash = 0;

    for(int i=0; i<6; i++) hash += mac_address[i] << (i+1);

    return(hash);
  }
}

/* *************************************** */

void Host::incrContact(Host *_peer, bool contacted_peer_as_client) {
  if(_peer->get_ip() != NULL)
    ((GenericHost*)this)->incrContact(iface, get_host_serial(), _peer->get_ip(),
				      contacted_peer_as_client);
}

/* *************************************** */

void Host::incStats(u_int8_t l4_proto, u_int ndpi_proto,
		    u_int64_t sent_packets, u_int64_t sent_bytes,
		    u_int64_t rcvd_packets, u_int64_t rcvd_bytes) {

  if(sent_packets || rcvd_packets) {
    ((GenericHost*)this)->incStats(l4_proto, ndpi_proto, sent_packets,
				   sent_bytes, rcvd_packets, rcvd_bytes);


    if(sent_packets == 1) sent_stats.incStats((u_int)sent_bytes);
    if(rcvd_packets == 1) recv_stats.incStats((u_int)rcvd_bytes);

    switch(l4_proto) {
    case 0:
      /* Unknown protocol */
      break;
    case IPPROTO_UDP:
      udp_rcvd.incStats(rcvd_packets, rcvd_bytes),
	udp_sent.incStats(sent_packets, sent_bytes);
      break;
    case IPPROTO_TCP:
      tcp_rcvd.incStats(rcvd_packets, rcvd_bytes),
	tcp_sent.incStats(sent_packets, sent_bytes);
      break;
    case IPPROTO_ICMP:
      icmp_rcvd.incStats(rcvd_packets, rcvd_bytes),
	icmp_sent.incStats(sent_packets, sent_bytes);
      break;
    default:
      other_ip_rcvd.incStats(rcvd_packets, rcvd_bytes),
	other_ip_sent.incStats(sent_packets, sent_bytes);
      break;
    }
  }
}

/* *************************************** */

char* Host::get_string_key(char *buf, u_int buf_len) {
  if(ip != NULL)
    return(ip->print(buf, buf_len));
  else
    return(get_mac(buf, buf_len));
}

/* *************************************** */

char* Host::serialize() {
  json_object *my_object;
  char *rsp, buf[32];

  my_object = json_object_new_object();

  json_object_object_add(my_object, "mac_address", json_object_new_string(get_mac(buf, sizeof(buf))));

  json_object_object_add(my_object, "asn", json_object_new_int(asn));
  if(symbolic_name)       json_object_object_add(my_object, "symbolic_name", json_object_new_string(symbolic_name));
  if(country)             json_object_object_add(my_object, "country",   json_object_new_string(country));
  if(city)                json_object_object_add(my_object, "city",      json_object_new_string(city));
  if(asname)              json_object_object_add(my_object, "asname",    json_object_new_string(asname));
  if(category[0] != '\0') json_object_object_add(my_object, "category",  json_object_new_string(category));
  if(httpbl[0] != '\0')   json_object_object_add(my_object, "httpbl",    json_object_new_string(httpbl));
  if(vlan_id != 0)        json_object_object_add(my_object, "vlan_id",   json_object_new_int(vlan_id));
  if(latitude)            json_object_object_add(my_object, "latitude",  json_object_new_double(latitude));
  if(longitude)           json_object_object_add(my_object, "longitude", json_object_new_double(longitude));
  if(ip)                  json_object_object_add(my_object, "ip", ip->getJSONObject());
  json_object_object_add(my_object, "localHost", json_object_new_boolean(localHost));
  json_object_object_add(my_object, "systemHost", json_object_new_boolean(systemHost));
  json_object_object_add(my_object, "tcp_sent", tcp_sent.getJSONObject());
  json_object_object_add(my_object, "tcp_rcvd", tcp_rcvd.getJSONObject());
  json_object_object_add(my_object, "udp_sent", udp_sent.getJSONObject());
  json_object_object_add(my_object, "udp_rcvd", udp_rcvd.getJSONObject());
  json_object_object_add(my_object, "icmp_sent", icmp_sent.getJSONObject());
  json_object_object_add(my_object, "icmp_rcvd", icmp_rcvd.getJSONObject());
  json_object_object_add(my_object, "other_ip_sent", other_ip_sent.getJSONObject());
  json_object_object_add(my_object, "other_ip_rcvd", other_ip_rcvd.getJSONObject());
  json_object_object_add(my_object, "pktStats.sent", sent_stats.getJSONObject());
  json_object_object_add(my_object, "pktStats.recv", recv_stats.getJSONObject());
  json_object_object_add(my_object, "throughput_bps", json_object_new_double(bytes_thpt));
  json_object_object_add(my_object, "throughput_trend_bps", json_object_new_string(Utils::trend2str(bytes_thpt_trend)));
  json_object_object_add(my_object, "throughput_pps", json_object_new_double(pkts_thpt));
  json_object_object_add(my_object, "throughput_trend_pps", json_object_new_string(Utils::trend2str(pkts_thpt_trend)));
  json_object_object_add(my_object, "flows.as_client", json_object_new_int(num_flows_as_client));
  json_object_object_add(my_object, "flows.as_server", json_object_new_int(num_flows_as_server));

  /* Generic Host */
  json_object_object_add(my_object, "num_alerts", json_object_new_int(getNumAlerts()));
  json_object_object_add(my_object, "sent", sent.getJSONObject());
  json_object_object_add(my_object, "rcvd", rcvd.getJSONObject());
  json_object_object_add(my_object, "ndpiStats", ndpiStats->getJSONObject(iface));
  json_object_object_add(my_object, "contacts", contacts->getJSONObject());
  json_object_object_add(my_object, "activityStats", activityStats.getJSONObject());

  if(dns) json_object_object_add(my_object, "dns", dns->getJSONObject());
  if(epp) json_object_object_add(my_object, "epp", epp->getJSONObject());

  //ntop->getTrace()->traceEvent(TRACE_WARNING, "%s()", __FUNCTION__);
  rsp = strdup(json_object_to_json_string(my_object));

  /* Free memory */
  json_object_put(my_object);

  return(rsp);
}

/* *************************************** */

bool Host::addIfMatching(lua_State* vm, char *key) {
  char keybuf[32] = { 0 } , *r;

  if(symbolic_name && strcasestr(symbolic_name, key)) {
    lua_push_str_table_entry(vm, get_string_key(keybuf, sizeof(keybuf)), symbolic_name);
    return(true);
  } else if(alternate_name && strcasestr(alternate_name, key)) {
    lua_push_str_table_entry(vm, get_string_key(keybuf, sizeof(keybuf)), alternate_name);
    return(true);
  } else if(strcasestr((r = get_mac(keybuf, sizeof(keybuf))), key)) {
    lua_push_str_table_entry(vm, get_string_key(keybuf, sizeof(keybuf)), r);
    return(true);
  } else if(ip && strcasestr((r = ip->print(keybuf, sizeof(keybuf))), key)) {
    lua_push_str_table_entry(vm, get_string_key(keybuf, sizeof(keybuf)), r);
    return(true);
  }

  return(false);
}

/* *************************************** */

bool Host::deserialize(char *json_str) {
  json_object *o, *obj;

  if((o = json_tokener_parse(json_str)) == NULL) return(false);

  if(json_object_object_get_ex(o, "mac_address", &obj)) set_mac((char*)json_object_get_string(obj));
  if(json_object_object_get_ex(o, "asn", &obj)) asn = json_object_get_int(obj);
  if(json_object_object_get_ex(o, "source_id", &obj)) source_id = json_object_get_int(obj);

  if(json_object_object_get_ex(o, "symbolic_name", &obj))  { if(symbolic_name) free(symbolic_name); symbolic_name = strdup(json_object_get_string(obj)); }
  if(json_object_object_get_ex(o, "country", &obj))        { if(country) free(country); country = strdup(json_object_get_string(obj)); }
  if(json_object_object_get_ex(o, "city", &obj))           { if(city) free(city); city = strdup(json_object_get_string(obj)); }
  if(json_object_object_get_ex(o, "asname", &obj))         { if(asname) free(asname); asname = strdup(json_object_get_string(obj)); }
  if(json_object_object_get_ex(o, "category", &obj))       { snprintf(category, sizeof(category), "%s", json_object_get_string(obj)); }
  if(json_object_object_get_ex(o, "httpbl", &obj))         { snprintf(httpbl, sizeof(httpbl), "%s", json_object_get_string(obj)); }
  if(json_object_object_get_ex(o, "vlan_id", &obj))   vlan_id = json_object_get_int(obj);
  if(json_object_object_get_ex(o, "latitude", &obj))  latitude  = (float)json_object_get_double(obj);
  if(json_object_object_get_ex(o, "longitude", &obj)) longitude = (float)json_object_get_double(obj);
  if(json_object_object_get_ex(o, "ip", &obj))  { if(ip == NULL) ip = new IpAddress(); if(ip) ip->deserialize(obj); }
  if(json_object_object_get_ex(o, "localHost", &obj)) localHost = (json_object_get_boolean(obj) ? true : false);
  if(json_object_object_get_ex(o, "systemHost", &obj)) systemHost = (json_object_get_boolean(obj) ? true : false);
  if(json_object_object_get_ex(o, "tcp_sent", &obj))  tcp_sent.deserialize(obj);
  if(json_object_object_get_ex(o, "tcp_rcvd", &obj))  tcp_rcvd.deserialize(obj);
  if(json_object_object_get_ex(o, "udp_sent", &obj))  udp_sent.deserialize(obj);
  if(json_object_object_get_ex(o, "udp_rcvd", &obj))  udp_rcvd.deserialize(obj);
  if(json_object_object_get_ex(o, "icmp_sent", &obj))  icmp_sent.deserialize(obj);
  if(json_object_object_get_ex(o, "icmp_rcvd", &obj))  icmp_rcvd.deserialize(obj);
  if(json_object_object_get_ex(o, "other_ip_sent", &obj))  other_ip_sent.deserialize(obj);
  if(json_object_object_get_ex(o, "other_ip_rcvd", &obj))  other_ip_rcvd.deserialize(obj);
  if(json_object_object_get_ex(o, "flows.as_client", &obj))  num_flows_as_client = json_object_get_int(obj);
  if(json_object_object_get_ex(o, "flows.as_server", &obj))  num_flows_as_server = json_object_get_int(obj);

  if(json_object_object_get_ex(o, "num_alerts", &obj)) num_alerts_detected = json_object_get_int(obj);
  if(json_object_object_get_ex(o, "sent", &obj))  sent.deserialize(obj);
  if(json_object_object_get_ex(o, "rcvd", &obj))  rcvd.deserialize(obj);

  if(json_object_object_get_ex(o, "dns", &obj)) {
    allocDNS();
    if(dns) dns->deserialize(obj); 
  }

  if(json_object_object_get_ex(o, "epp", &obj)) {
    allocEPP();
    if(epp) epp->deserialize(obj); 
  }

  if(ndpiStats) { delete ndpiStats; ndpiStats = NULL; }
  if(json_object_object_get_ex(o, "ndpiStats", &obj)) { ndpiStats = new NdpiStats(); ndpiStats->deserialize(iface, obj); }

  activityStats.reset();
  if(json_object_object_get_ex(o, "activityStats", &obj)) activityStats.deserialize(obj);

  computeHostSerial();
  if(json_object_object_get_ex(o, "contacts", &obj)) contacts->deserialize(iface, this, obj);
  if(json_object_object_get_ex(o, "pktStats.sent", &obj)) sent_stats.deserialize(obj);
  if(json_object_object_get_ex(o, "pktStats.recv", &obj)) recv_stats.deserialize(obj);

  json_object_put(o);

  /* We need to update too the stats for traffic */
  last_update_time.tv_sec = (long)time(NULL), last_update_time.tv_usec = 0;
  // Update bps throughput
  bytes_thpt = 0, last_bytes = sent.getNumBytes()+rcvd.getNumBytes(),
  bytes_thpt_trend = trend_unknown;

  // Update pps throughput
  pkts_thpt = 0, last_packets = sent.getNumPkts()+rcvd.getNumPkts(),
  pkts_thpt_trend = trend_unknown;

  return(true);
}

/* *************************************** */

void Host::updateSynFlags(time_t when, u_int8_t flags, Flow *f) {
  if(syn_flood_alert->incHits(when)) {
    char ip_buf[48], flow_buf[256], msg[512], *h;

    if(!triggerAlerts()) return;

    /* 
       It's normal that at startup several flows are created
    */
    if(ntop->getUptime() < 10 /* sec */) return;

    h = ip->print(ip_buf, sizeof(ip_buf));
    snprintf(msg, sizeof(msg),
	     "Host <A HREF=/lua/host_details.lua?host=%s&ifname=%s>%s</A> is a SYN flooder [%u SYNs in the last %u sec] %s", 
	     h, iface->get_name(), h,
	     syn_flood_alert->getCurrentHits(),
	     syn_flood_alert->getOverThresholdDuration(),
	     f->print(flow_buf, sizeof(flow_buf)));
    
    ntop->getTrace()->traceEvent(TRACE_INFO, "SYN Flood: %s", msg);
    ntop->getRedis()->queueAlert(alert_level_error, alert_syn_flood, msg);
    incNumAlerts();
  }  
}

