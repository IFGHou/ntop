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

HostContacts::HostContacts() {
  memset(clientContacts, 0, sizeof(clientContacts));
  memset(serverContacts, 0, sizeof(serverContacts));
}

/* *************************************** */

void HostContacts::incrIPContacts(NetworkInterface *iface,
				  u_int32_t me_serial,
				  IpAddress *peer, bool contacted_peer_as_client,
				  IPContacts *contacts, u_int32_t value,
				  u_int family_id,
				  bool aggregated_host) {
  int8_t    least_idx = -1;
  u_int32_t least_value = 0;

  if(value == 0)
    ntop->getTrace()->traceEvent(TRACE_WARNING, "%s(): zero contacts", __FUNCTION__);

  for(int i=0; i<MAX_NUM_HOST_CONTACTS; i++) {
    if(contacts[i].num_contacts == 0) {
      /* Empty slot */
      contacts[i].host.set(peer), contacts[i].num_contacts = value;
      return;
    } else if(contacts[i].host.compare(peer) == 0) {
      contacts[i].num_contacts += value;
      return;
    } else {
      if((least_idx == -1) || (least_value > contacts[i].num_contacts))
	least_idx = i, least_value = contacts[i].num_contacts;
    }
  } /* for */

  /* No room found: let's discard the item with lowest score */
  if(Utils::dumpHostToDB(&contacts[least_idx].host,
			 ntop->getPrefs()->get_dump_hosts_to_db_policy())) {
    if(me_serial > 0) {
      /* This is a host */
      char daybuf[64];
      time_t when = time(NULL);
      bool new_key;
      char buf[64];
      u_int32_t peer_serial;
      
      strftime(daybuf, sizeof(daybuf), CONST_DB_DAY_FORMAT, localtime(&when));
      peer_serial = ntop->getRedis()->host_to_id(iface, daybuf, 
						 contacts[least_idx].host.print(buf, sizeof(buf)), &new_key);

      if(contacted_peer_as_client)
	dbDumpHost(daybuf, iface, me_serial, peer_serial,
		   family_id, contacts[least_idx].num_contacts);
      else
	dbDumpHost(daybuf, iface, peer_serial, me_serial, 
		   family_id, contacts[least_idx].num_contacts);
    } else {
      /* This is an aggregation */

      ntop->getTrace()->traceEvent(TRACE_ERROR, "%s(): zero serial", __FUNCTION__);
    }
  }

  contacts[least_idx].host.set(peer),
    contacts[least_idx].num_contacts = value;
}

/* *************************************** */

bool HostContacts::hasHostContacts(char *host) {
  char buf[64];
  
  if(host == NULL) return(false);

  for(int i=0; i<MAX_NUM_HOST_CONTACTS; i++) {
    if((clientContacts[i].num_contacts > 0)
       && (!strcmp(clientContacts[i].host.print(buf, sizeof(buf)), host)))
      return(true);

    if((serverContacts[i].num_contacts > 0)
       && (!strcmp(serverContacts[i].host.print(buf, sizeof(buf)), host)))
      return(true);
  }

  return(false);
}

/* *************************************** */

void HostContacts::getContacts(lua_State* vm) {
  char buf[64];

  lua_newtable(vm);

  /* client */
  lua_newtable(vm);
  for(int i=0; i<MAX_NUM_HOST_CONTACTS; i++) {
    if(clientContacts[i].num_contacts > 0)
      lua_push_int_table_entry(vm,
			       clientContacts[i].host.print(buf, sizeof(buf)),
			       clientContacts[i].num_contacts);
  }
  lua_pushstring(vm, "client");
  lua_insert(vm, -2);
  lua_settable(vm, -3);

  /* server */
  lua_newtable(vm);
  for(int i=0; i<MAX_NUM_HOST_CONTACTS; i++) {
    if(serverContacts[i].num_contacts > 0)
      lua_push_int_table_entry(vm,
			       serverContacts[i].host.print(buf, sizeof(buf)),
			       serverContacts[i].num_contacts);
  }
  lua_pushstring(vm, "server");
  lua_insert(vm, -2);
  lua_settable(vm, -3);

  lua_pushstring(vm, "contacts");
  lua_insert(vm, -2);
  lua_settable(vm, -3);
}

/* *************************************** */

char* HostContacts::serialize() {
  json_object *my_object;
  json_object *inner;
  char *rsp;

  my_object = json_object_new_object();

  /* *************************************** */

  inner = json_object_new_object();

  for(int i=0; i<MAX_NUM_HOST_CONTACTS; i++) {
    if(clientContacts[i].num_contacts > 0) {
      char buf[64], buf2[32];

      snprintf(buf2, sizeof(buf2), "%u", clientContacts[i].num_contacts);
      json_object_object_add(inner, clientContacts[i].host.print(buf, sizeof(buf)), json_object_new_string(buf2));
    }
  }

  json_object_object_add(my_object, "client", inner);

  /* *************************************** */

  inner = json_object_new_object();

  for(int i=0; i<MAX_NUM_HOST_CONTACTS; i++) {
    if(serverContacts[i].num_contacts > 0) {
      char buf[64], buf2[32];

      snprintf(buf2, sizeof(buf2), "%u", serverContacts[i].num_contacts);
      json_object_object_add(inner, serverContacts[i].host.print(buf, sizeof(buf)), json_object_new_string(buf2));
    }
  }

  json_object_object_add(my_object, "server", inner);

  /* *************************************** */

  rsp = strdup(json_object_to_json_string(my_object));

  // ntop->getTrace()->traceEvent(TRACE_WARNING, "%s", rsp);

  /* Free memory */
  json_object_put(my_object);

  return(rsp);
}

/* *************************************** */

void HostContacts::deserialize(NetworkInterface *iface, GenericHost *h, json_object *o) {
  json_object *obj;
  IpAddress ip;

  if(!o) return;

  /* Reset all */
  memset(clientContacts, 0, sizeof(clientContacts));
  memset(serverContacts, 0, sizeof(serverContacts));

  if(json_object_object_get_ex(o, "client", &obj)) {
    struct json_object_iterator it = json_object_iter_begin(obj);
    struct json_object_iterator itEnd = json_object_iter_end(obj);

    while (!json_object_iter_equal(&it, &itEnd)) {
      char *key  = (char*)json_object_iter_peek_name(&it);
      int  value = json_object_get_int(json_object_iter_peek_value(&it));

      ip.set_from_string(key);
      incrContact(iface, h->get_host_serial(), &ip, true /* client */, value, HOST_FAMILY_ID, false);

      //ntop->getTrace()->traceEvent(TRACE_WARNING, "%s=%d", key, value);

      json_object_iter_next(&it);
    }
  }

  if(json_object_object_get_ex(o, "server", &obj)) {
    struct json_object_iterator it = json_object_iter_begin(obj);
    struct json_object_iterator itEnd = json_object_iter_end(obj);

    while (!json_object_iter_equal(&it, &itEnd)) {
      char *key  = (char*)json_object_iter_peek_name(&it);
      int  value = json_object_get_int(json_object_iter_peek_value(&it));

      ip.set_from_string(key);
      incrContact(iface, h->get_host_serial(), &ip, false /* server */, value, HOST_FAMILY_ID, false);

      // ntop->getTrace()->traceEvent(TRACE_WARNING, "%s=%d", key, value);

      json_object_iter_next(&it);
    }
  }
}

/* *************************************** */

json_object* HostContacts::getJSONObject() {
  char *s = serialize();
  json_object *o = json_tokener_parse(s);

  free(s);
  return(o);
}

/* *************************************** */

u_int HostContacts::get_num_contacts_by(IpAddress* host_ip) {
  u_int num = 0;

  for(int i=0; i<MAX_NUM_HOST_CONTACTS; i++) {
    if(clientContacts[i].num_contacts == 0) break;

    if(clientContacts[i].host.equal(host_ip))
      num += clientContacts[i].num_contacts;
  }

  return(num);
}

/* *************************************** */

u_int8_t HostContacts::get_queue_id(char *str) {
  int id = 0, len = strlen(str);

  for(int i=0; i<len; i++) id += str[i];
  return(id % CONST_NUM_OPEN_DB_CACHE);
}

/* *************************************** */

char* HostContacts::get_cache_key(char *daybuf, char *ifname, 
				  u_int32_t host_id,
				  bool client_mode,
				  char *buf, u_int buf_len) {
  /* <date>|<iface>|<host IP>|<CONST_CONTACTED_BY|CONST_CONTACTS> <peer IP> <value> */

  snprintf(buf, buf_len, "%s|%s|%u|%s",
	   daybuf, ifname, host_id,
	   client_mode ? CONST_CONTACTS : CONST_CONTACTED_BY);
  return(buf);
}

/* *************************************** */

void HostContacts::dbDumpHost(char *daybuf, NetworkInterface *iface,
			      u_int32_t host_id,
			      u_int32_t peer_id, u_int family_id,
			      u_int32_t num_contacts) {
  char full_path[MAX_PATH];
  char *ifname = iface->get_name();
  char *k = get_cache_key(daybuf, ifname,
			  host_id,
			  true /* client */,
			  full_path, sizeof(full_path));

  ntop->getRedis()->incrHostContacts(k, family_id, peer_id, num_contacts);
}

/* *************************************** */

#if 0
void HostContacts::dbDumpSingleHost(char *daybuf, 
				    NetworkInterface *iface, 
				    u_int32_t client_serial,
				    u_int32_t server_serial,
				    u_int16_t family_id,
				    u_int32_t num_contacts) {

}
#endif

/* *************************************** */

#define ifdot(a) ((a == '.') ? '_' : a)

void HostContacts::dbDumpAllHosts(char *daybuf, NetworkInterface *iface,
				  u_int32_t host_id, u_int16_t family_id) {
  char *ifname = iface->get_name();
  char buf[64], cmd[MAX_PATH], *k;

  for(int i=0; i<MAX_NUM_HOST_CONTACTS; i++) {
    if(clientContacts[i].num_contacts == 0)
      break;

    if(clientContacts[i].num_contacts > 0) {
      if(Utils::dumpHostToDB(&clientContacts[i].host,
			     (family_id == HOST_FAMILY_ID) ?
			     ntop->getPrefs()->get_dump_hosts_to_db_policy() :
			     ntop->getPrefs()->get_dump_aggregations_to_db())) {
	char *peer_ip = clientContacts[i].host.print(buf, sizeof(buf));
	bool new_key;
	u_int32_t peer_serial = ntop->getRedis()->host_to_id(iface, daybuf, peer_ip, &new_key);

	k = get_cache_key(daybuf, ifname,
			  host_id,
			  (family_id == HOST_FAMILY_ID) ? true : false,
			  cmd, sizeof(cmd));
	ntop->getRedis()->incrHostContacts(k, family_id, peer_serial, clientContacts[i].num_contacts);

	if(family_id != HOST_FAMILY_ID) {
	  k = get_cache_key(daybuf, ifname,
			    peer_serial,
			    true, cmd, sizeof(cmd));
	  ntop->getRedis()->incrHostContacts(k, family_id, host_id, clientContacts[i].num_contacts);
	}
      }
    }

    if(serverContacts[i].num_contacts > 0) {
      if(Utils::dumpHostToDB(&serverContacts[i].host,
			     (family_id == HOST_FAMILY_ID) ?
			     ntop->getPrefs()->get_dump_hosts_to_db_policy() :
			     ntop->getPrefs()->get_dump_aggregations_to_db())) {
	bool new_key;
	char *peer_ip = serverContacts[i].host.print(buf, sizeof(buf));
	u_int32_t peer_serial = ntop->getRedis()->host_to_id(iface, daybuf, peer_ip, &new_key);

	k = get_cache_key(daybuf, ifname,
			  host_id, false, cmd, sizeof(cmd));
	ntop->getRedis()->incrHostContacts(k, family_id, peer_serial, serverContacts[i].num_contacts);

	if(family_id != HOST_FAMILY_ID) {
	  k = get_cache_key(daybuf, ifname,
			    peer_serial, true, cmd, sizeof(cmd));
	  ntop->getRedis()->incrHostContacts(k, family_id, host_id, serverContacts[i].num_contacts);
	}
      } /* if */
    } /* if */
  }
}

/* *************************************** */

void HostContacts::purgeAll() {
  /* Perhaps we should add a lock... */
  memset(clientContacts, 0, sizeof(IPContacts)*MAX_NUM_HOST_CONTACTS);
  memset(serverContacts, 0, sizeof(IPContacts)*MAX_NUM_HOST_CONTACTS);
}
