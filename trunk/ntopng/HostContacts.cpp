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

/* *************************************** */

HostContacts::HostContacts() {
  memset(clientContacts, 0, sizeof(IPContacts)*MAX_NUM_HOST_CONTACTS);
  memset(serverContacts, 0, sizeof(IPContacts)*MAX_NUM_HOST_CONTACTS);
}

/* *************************************** */

HostContacts::~HostContacts() {
  for(int i=0; i<MAX_NUM_HOST_CONTACTS; i++) {
    if(clientContacts[i].host != NULL) delete clientContacts[i].host;
    if(serverContacts[i].host != NULL) delete serverContacts[i].host;
  }
}

/* *************************************** */

bool HostContacts::dumpHostToDB(IpAddress *host, LocationPolicy policy) {
  bool do_dump = false;
  
  switch(policy) {
  case location_local_only:
    if(host->isLocalHost()) do_dump = true;
    break;
  case location_remote_only:
    if(!host->isLocalHost()) do_dump = true;
    break;
  case location_all:
    do_dump = true;
    break;
  case location_none:
    do_dump = false;
    break;
  }

  return(do_dump);
}

/* *************************************** */

void HostContacts::incrIPContacts(NetworkInterface *iface, 
				  IpAddress *me, char *me_name,
				  IpAddress *peer,
				  IPContacts *contacts, u_int32_t value,
				  bool aggregated_host) {
  int8_t    least_idx = -1;
  u_int32_t least_value = 0;

  if(value == 0)
    ntop->getTrace()->traceEvent(TRACE_WARNING, "%s(): zero contacts", __FUNCTION__);
  
  for(int i=0; i<MAX_NUM_HOST_CONTACTS; i++) {
    if(contacts[i].host == NULL) {
      /* Empty slot */
      contacts[i].host = new IpAddress(peer), contacts[i].num_contacts = value;
      return;
    } else if(contacts[i].host->compare(peer) == 0) {
      contacts[i].num_contacts += value;
      return;
    } else {
      if((least_idx == -1) || (least_value > contacts[i].num_contacts))
	least_idx = i, least_value = contacts[i].num_contacts;
    }
  } /* for */

  /* No room found: let's discard the item with lowest score */  
  if(dumpHostToDB(contacts[least_idx].host, ntop->getPrefs()->get_dump_hosts_to_db_policy())) {
    if(me != NULL) {
	/* This is a host */
      char daybuf[64];
      char me_key[128], *me_k = me->print(me_key, sizeof(me_key));
      time_t when = time(NULL);
      
      strftime(daybuf, sizeof(daybuf), "%y/%m/%d", localtime(&when));
      dbDumpHost(daybuf, iface->get_name(), me_k, contacts[least_idx].host, contacts[least_idx].num_contacts);
    } else {
      /* This is an aggregation */
      
    }
  }

  delete contacts[least_idx].host;
  contacts[least_idx].host = new IpAddress(peer), 
    contacts[least_idx].num_contacts = value;
}

/* *************************************** */

void HostContacts::getIPContacts(lua_State* vm) {
  char buf[64];

  lua_newtable(vm);

  /* client */
  lua_newtable(vm);
  for(int i=0; i<MAX_NUM_HOST_CONTACTS; i++) {
    if(clientContacts[i].host != NULL)
      lua_push_int_table_entry(vm,
			       clientContacts[i].host->print(buf, sizeof(buf)),
			       clientContacts[i].num_contacts);
  }
  lua_pushstring(vm, "client");
  lua_insert(vm, -2);
  lua_settable(vm, -3);

  /* server */
  lua_newtable(vm);
  for(int i=0; i<MAX_NUM_HOST_CONTACTS; i++) {
    if(serverContacts[i].host != NULL)
      lua_push_int_table_entry(vm,
			       serverContacts[i].host->print(buf, sizeof(buf)),
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
    if(clientContacts[i].host != NULL) {
      char buf[64], buf2[32];

      snprintf(buf2, sizeof(buf2), "%u", clientContacts[i].num_contacts);
      json_object_object_add(inner, clientContacts[i].host->print(buf, sizeof(buf)), json_object_new_string(buf2));
    }
  }

  json_object_object_add(my_object, "client", inner);

  /* *************************************** */

  inner = json_object_new_object();

  for(int i=0; i<MAX_NUM_HOST_CONTACTS; i++) {
    if(serverContacts[i].host != NULL) {
      char buf[64], buf2[32];

      snprintf(buf2, sizeof(buf2), "%u", serverContacts[i].num_contacts);
      json_object_object_add(inner, serverContacts[i].host->print(buf, sizeof(buf)), json_object_new_string(buf2));
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
  memset(clientContacts, 0, sizeof(IPContacts)*MAX_NUM_HOST_CONTACTS);
  memset(serverContacts, 0, sizeof(IPContacts)*MAX_NUM_HOST_CONTACTS);

  if(json_object_object_get_ex(o, "client", &obj)) {
    struct json_object_iterator it = json_object_iter_begin(obj);
    struct json_object_iterator itEnd = json_object_iter_end(obj);

    while (!json_object_iter_equal(&it, &itEnd)) {
      char *key  = (char*)json_object_iter_peek_name(&it);
      int  value = json_object_get_int(json_object_iter_peek_value(&it));

      ip.set_from_string(key);
      incrContact(iface, (char*)NULL, &ip, true /* client */, value, false);

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
      incrContact(iface, (char*)NULL, &ip, false /* server */, value, false);

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

    if(clientContacts[i].host->equal(host_ip))
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

void HostContacts::dbDumpHost(char *daybuf, char *ifname, char *key, 
			      IpAddress *peer, u_int32_t num_contacts) {
  char buf[32], full_path[MAX_PATH];
  char *host_ip = peer->print(buf, sizeof(buf));

  snprintf(full_path, sizeof(full_path), "%s/%s/%s/%c/%c|%s",
	   ntop->get_working_dir(), ifname, CONST_HOST_CONTACTS,
	   key[0], ifdot(key[1]), key);
  ntop->fixPath(full_path);

  ntop->getRedis()->queueContactToDump(full_path, true, get_queue_id(key),
				       host_ip, HOST_FAMILY_ID, num_contacts);  
}

/* *************************************** */

#define ifdot(a) ((a == '.') ? '_' : a)

void HostContacts::dbDump(char *daybuf, char *ifname, char *key, u_int16_t family_id) {
#if 0
  char buf[64], cmd[256];

  for(int i=0; i<MAX_NUM_HOST_CONTACTS; i++) {
    if(clientContacts[i].host != NULL) {
      if(dumpHostToDB(clientContacts[i].host,
		      (family_id == HOST_FAMILY_ID) ?
		      ntop->getPrefs()->get_dump_hosts_to_db_policy() :
		      ntop->getPrefs()->get_dump_aggregations_to_db())) {
	char *host_ip = clientContacts[i].host->print(buf, sizeof(buf));

	snprintf(cmd, sizeof(cmd), "HINCR %s-%s-%s-%u-%s %s %u",
		 daybuf, ifname, key, family_id,
		 (family_id == HOST_FAMILY_ID) ? CONST_CONTACTS : CONST_CONTACTED_BY,
		 host_ip, clientContacts[i].num_contacts);
	ntop->getTrace()->traceEvent(TRACE_NORMAL, "%s", cmd);

	if(family_id != HOST_FAMILY_ID) {
	  snprintf(cmd, sizeof(cmd), "HINCR %s-%s-%s-%u-%s %s %u",
		   daybuf, ifname, host_ip, family_id,
		   CONST_CONTACTS,
		   key, clientContacts[i].num_contacts);
	  ntop->getTrace()->traceEvent(TRACE_NORMAL, "%s", cmd);
	}
      }
    }

    if(serverContacts[i].host != NULL) {
      if(dumpHostToDB(serverContacts[i].host,
		      (family_id == HOST_FAMILY_ID) ?
		      ntop->getPrefs()->get_dump_hosts_to_db_policy() :
		      ntop->getPrefs()->get_dump_aggregations_to_db())) {
	char *host_ip = serverContacts[i].host->print(buf, sizeof(buf));

	snprintf(cmd, sizeof(cmd), "HINCR %s-%s-%s-%u-%s %s %u",
		 daybuf, ifname, key, family_id, CONST_CONTACTED_BY,
		 host_ip, serverContacts[i].num_contacts);
	ntop->getTrace()->traceEvent(TRACE_NORMAL, "%s", cmd);

	if(family_id != HOST_FAMILY_ID) {
	  snprintf(cmd, sizeof(cmd), "HINCR %s-%s-%s-%u-%s %s %u",
		   daybuf, ifname, host_ip, family_id,
		   CONST_HOST_CONTACTS,
		   key, serverContacts[i].num_contacts);
	  ntop->getTrace()->traceEvent(TRACE_NORMAL, "%s", cmd);
	}
      } /* if */
    } /* if */
  }
#else
  char buf[64], full_path[MAX_PATH], alt_full_path[MAX_PATH];

  snprintf(full_path, sizeof(full_path), "%s/%s/%s/%c/%c|%s",
	   ntop->get_working_dir(), ifname, 
	   (family_id == HOST_FAMILY_ID) ? CONST_HOST_CONTACTS : CONST_AGGREGATIONS,
	   key[0], ifdot(key[1]), key);
  ntop->fixPath(full_path);

  for(int i=0; i<MAX_NUM_HOST_CONTACTS; i++) {
    if(clientContacts[i].host != NULL) {

      if(dumpHostToDB(clientContacts[i].host,
		      (family_id == HOST_FAMILY_ID) ?
		      ntop->getPrefs()->get_dump_hosts_to_db_policy() :
		      ntop->getPrefs()->get_dump_aggregations_to_db())) {
	char *host_ip = clientContacts[i].host->print(buf, sizeof(buf));
	ntop->getRedis()->queueContactToDump(full_path, 
					     (family_id == HOST_FAMILY_ID) ? true : false, 
					     get_queue_id(key),
					     host_ip, family_id, 
					     clientContacts[i].num_contacts);
	
	if(family_id != HOST_FAMILY_ID) {
	  snprintf(alt_full_path, sizeof(alt_full_path), "%s/%s/%s/%c/%c|%s",
		   ntop->get_working_dir(), ifname, 
		   CONST_HOST_CONTACTS, host_ip[0], ifdot(host_ip[1]), host_ip);
	  ntop->fixPath(alt_full_path);
	  ntop->getRedis()->queueContactToDump(alt_full_path, true, get_queue_id(host_ip),
					       key, family_id, 
					       clientContacts[i].num_contacts);
	}
      }
    }

    if(serverContacts[i].host != NULL) {
      if(dumpHostToDB(serverContacts[i].host,
		      (family_id == HOST_FAMILY_ID) ?
		      ntop->getPrefs()->get_dump_hosts_to_db_policy() :
		      ntop->getPrefs()->get_dump_aggregations_to_db())) {
	char *host_ip;

	host_ip = serverContacts[i].host->print(buf, sizeof(buf));
	ntop->getRedis()->queueContactToDump(full_path, false, get_queue_id(key),
					     host_ip, family_id, 
					     serverContacts[i].num_contacts);
	
	if(family_id != HOST_FAMILY_ID) {
	  snprintf(alt_full_path, sizeof(alt_full_path), "%s/%s/%s/%c/%c|%s",
		   ntop->get_working_dir(), ifname, 
		   CONST_HOST_CONTACTS, host_ip[0], ifdot(host_ip[1]), host_ip);
	  ntop->fixPath(alt_full_path);
	  ntop->getRedis()->queueContactToDump(alt_full_path, true, get_queue_id(host_ip),
					       key, family_id,
					       serverContacts[i].num_contacts);
	}
      } /* if */
    } /* if */
  } /* for */
#endif
}

