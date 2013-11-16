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

/* **************************************** */

Redis::Redis(char *redis_host, int redis_port) {
  REDIS_INFO info;
  int major, minor, sub;

  if(((redis = credis_connect(redis_host, redis_port, 10000)) == NULL)
     || (credis_ping(redis) != 0)
     || (credis_info(redis, &info) != 0)
     ) {
    ntop->getTrace()->traceEvent(TRACE_ERROR,
				 "Unable to connect to redis %s:%d",
				 redis_host, redis_port);
    ntop->getTrace()->traceEvent(TRACE_ERROR, "ntong requires redis server to be up and running");
    ntop->getTrace()->traceEvent(TRACE_ERROR, "Please start it and try again or use -r");
    ntop->getTrace()->traceEvent(TRACE_ERROR, "to specify a redis server other than the default");
    exit(-1);
  }

  if(sscanf((const char*)info.redis_version, "%d.%d.%d", &major, &minor, &sub) == 3) {
    u_int version = (major << 16) + (minor << 8) + sub;

    if(version < 0x00020200 /* 2.2.0 */) {
      ntop->getTrace()->traceEvent(TRACE_NORMAL,
				   "Your redis version is too old (%s): please use at least 2.2 or newer",
				   info.redis_version);
      exit(0);
    }
  }

  ntop->getTrace()->traceEvent(TRACE_NORMAL,
			       "Successfully connected to Redis %d bit v.%s",
			       info.arch_bits, info.redis_version);
  l = new Mutex();
  setDefaults();
}

/* **************************************** */

Redis::~Redis() {
  credis_close(redis);
  delete l;
}

/* **************************************** */

int Redis::expire(char *key, u_int expire_sec) {
  int rc;

  l->lock(__FILE__, __LINE__);
  rc = credis_expire(redis, key, expire_sec);
  l->unlock(__FILE__, __LINE__);

  return(rc);
}

/* **************************************** */

int Redis::get(char *key, char *rsp, u_int rsp_len) {
  char *val;
  int rc;

  l->lock(__FILE__, __LINE__);
  if((rc = credis_get(redis, key, &val)) == 0) {
    snprintf(rsp, rsp_len, "%s", val);
    // free(val);
  } else
    rsp[0] = 0;
  l->unlock(__FILE__, __LINE__);

  return(rc);
}

/* **************************************** */

int Redis::set(char *key, char *value, u_int expire_secs) {
  int rc;

  l->lock(__FILE__, __LINE__);
  rc = credis_set(redis, key, value);

  if((rc == 0) && (expire_secs != 0))
    credis_expire(redis, key, expire_secs);
  l->unlock(__FILE__, __LINE__);

  // ntop->getTrace()->traceEvent(TRACE_NORMAL, "%s <-> %s", key, value);
  return(rc);
}

/* **************************************** */

/*
  Incrememnt key.member of +value and keeps at most trim_len elements
*/
int Redis::zincrbyAndTrim(char *key, char *member, u_int value, u_int trim_len) {
  int rc;
  double new_val;

  l->lock(__FILE__, __LINE__);
  rc = credis_zincrby(redis, key, (double)value, member, &new_val);

  if((rc == 0) && (trim_len > 0))
    rc = credis_zremrangebyrank(redis, key, 0, -1*trim_len);
  l->unlock(__FILE__, __LINE__);

  // ntop->getTrace()->traceEvent(TRACE_NORMAL, "%s <-> %s", key, value);
  return(rc);
}

/* **************************************** */

int Redis::keys(const char *pattern, char ***keys_p) {
  char **keys;
  int rc, i;

  l->lock(__FILE__, __LINE__);
  rc = credis_keys(redis, pattern, &keys);
  if(rc > 0) {
    (*keys_p) = (char**) malloc(rc * sizeof(char*));
    if ((*keys_p) == NULL) rc = -1;
    else for (i = 0; i < rc; i++)
      (*keys_p)[i] = strdup(keys[i]);
  }
  l->unlock(__FILE__, __LINE__);

  return(rc);
}

/* **************************************** */

int Redis::del(char *key) {
  int rc;

  l->lock(__FILE__, __LINE__);
  rc = credis_del(redis, key);
  l->unlock(__FILE__, __LINE__);

  return(rc);
}

/* **************************************** */

int Redis::queueHostToResolve(char *hostname, bool dont_check_for_existance, bool localHost) {
  int rc;
  char key[128], *val;
  bool found;

  if(!ntop->getPrefs()->is_dns_resolution_enabled()) return(0);

  snprintf(key, sizeof(key), "%s.%s", DNS_CACHE, hostname);

  l->lock(__FILE__, __LINE__);

  if(dont_check_for_existance)
    found = false;
  else {
    /*
      Add only if the address has not been resolved yet
    */
    if(credis_get(redis, key, &val) < 0)
      found = false;
    else
      found = true;
  }

  if(!found) {
    /* Add to the list of addresses to resolve */
    if(localHost)
      rc = credis_lpush(redis, DNS_TO_RESOLVE, hostname);
    else
      rc = credis_rpush(redis, DNS_TO_RESOLVE, hostname);

    /*
      We make sure that no more than MAX_NUM_QUEUED_ADDRS entries are in queue
      This is important in order to avoid the cache to grow too much
    */
    credis_ltrim(redis, DNS_TO_RESOLVE, 0, MAX_NUM_QUEUED_ADDRS);
  } else
    rc = 0;

  l->unlock(__FILE__, __LINE__);

  return(rc);
}

/* **************************************** */

int Redis::popHostToResolve(char *hostname, u_int hostname_len) {
  char *val;
  int rc;

  l->lock(__FILE__, __LINE__);
  rc = credis_lpop(redis, (char*)DNS_TO_RESOLVE, &val);
  l->unlock(__FILE__, __LINE__);

  if(rc == 0)
    snprintf(hostname, hostname_len, "%s", val);
  else
    hostname[0] = '\0';

  return(rc);
}

/* **************************************** */

int Redis::getDNSQueueLength() {
  int rc;

  l->lock(__FILE__, __LINE__);
  rc = credis_llen(redis, (char*)DNS_TO_RESOLVE);
  l->unlock(__FILE__, __LINE__);

  return(rc);
}

/* **************************************** */

char* Redis::getFlowCategory(char *domainname, char *buf, u_int buf_len, bool categorize_if_unknown) {
  char key[128], *val;

  buf[0] = 0;

  if(!ntop->getPrefs()->is_categorization_enabled())  return(NULL);

  /* Check if the host is 'categorizable' */
  if(Utils::isIPAddress(domainname)) {
    return(buf);
  }

  l->lock(__FILE__, __LINE__);

  snprintf(key, sizeof(key), "%s.%s", DOMAIN_CATEGORY, domainname);

  /*
    Add only if the domain has not been categorized yet
  */
  if(credis_get(redis, key, &val) < 0) {
    if(categorize_if_unknown)
      /* rc = */ credis_rpush(redis, DOMAIN_TO_CATEGORIZE, domainname);

    buf[0] = 0, val = NULL;
  } else
    snprintf(buf, buf_len, "%s", val);

  l->unlock(__FILE__, __LINE__);

  return(val);
}

/* **************************************** */

int Redis::popDomainToCategorize(char *domainname, u_int domainname_len) {
  char *val;
  int rc;

  l->lock(__FILE__, __LINE__);
  rc = credis_lpop(redis, (char*)DOMAIN_TO_CATEGORIZE, &val);
  l->unlock(__FILE__, __LINE__);

  if(rc == 0)
    snprintf(domainname, domainname_len, "%s", val);
  else
    domainname[0] = '\0';

  return(rc);
}

/* **************************************** */

void Redis::setDefaults() {
  char value[32];

  setResolvedAddress((char*)"127.0.0.1", (char*)"localhost");
  setResolvedAddress((char*)"255.255.255.255", (char*)"Broadcast");
  setResolvedAddress((char*)"0.0.0.0", (char*)"NoIP");

  if(get((char*)"user.admin.password", value, sizeof(value)) < 0)
    set((char*)"user.admin.password", (char*)"21232f297a57a5a743894a0e4a801fc3");
}

/* **************************************** */

int Redis::getAddress(char *numeric_ip, char *rsp,
		      u_int rsp_len, bool queue_if_not_found) {
  char key[64];
  int rc;

  rsp[0] = '\0';
  snprintf(key, sizeof(key), "%s.%s", DNS_CACHE, numeric_ip);

 rc = get(key, rsp, rsp_len);

  if(rc != 0) {
    if(queue_if_not_found)
      queueHostToResolve(numeric_ip, true, false);
  } else {
    /* We need to extend expire */

    expire(numeric_ip, 300 /* expire */);
  }

  return(rc);
}

/* **************************************** */

int Redis::setResolvedAddress(char *numeric_ip, char *symbolic_ip) {
  char key[64];

  snprintf(key, sizeof(key), "%s.%s", DNS_CACHE, numeric_ip);
  return(set(key, symbolic_ip, 300));
}

/* **************************************** */

char* Redis::getVersion(char *str, u_int str_len) {
  REDIS_INFO info;

  l->lock(__FILE__, __LINE__);
  if(redis && (credis_info(redis, &info) == 0))
    snprintf(str, str_len, "%s (%d bit)", info.redis_version, info.arch_bits);
  else
    str[0] = 0;
  l->unlock(__FILE__, __LINE__);

  return(str);
}

/* **************************************** */

void Redis::getHostContacts(lua_State* vm, GenericHost *h, bool client_contacts) {
  int rc;
  char **rsp, hkey[64], key[64];

  h->get_string_key(hkey, sizeof(hkey));
  if(hkey[0] == '\0') return;

  snprintf(key, sizeof(key), "%s.%s", hkey,
	   client_contacts ? "client" : "server");

  lua_newtable(vm);

  l->lock(__FILE__, __LINE__);
  rc = credis_zrevrange(redis, key, 0, -1, 1 /* withscores */, &rsp);

  if(rc > 0) {
    for(int i=0; i<(rc-1); i++) {
      if((i % 2) == 0) {
	const char *key = (const char*)rsp[i];
	u_int64_t value = (u_int64_t)atol(rsp[i+1]);
	
	//ntop->getTrace()->traceEvent(TRACE_NORMAL, "%s:%llu", key, value);
	lua_push_int_table_entry(vm, key, value);
      }
    }
  }
  
  l->unlock(__FILE__, __LINE__);
}
 
/* **************************************** */

int Redis::queueContactToDump(char *path, bool client_mode, char *key, 
			      u_int16_t family_id, u_int32_t num_contacts) {
  int rc;
  char sql[1024];
  const char *table_name = client_mode ? "client_contacts" : "server_contacts";

  snprintf(sql, sizeof(sql),
	   "%s|INSERT OR IGNORE INTO %s VALUES ('%s', %u, 0);"
	   "UPDATE %s SET contacts=contacts+%u WHERE key='%s' AND family=%d;",
	   path, table_name, key, family_id, 
	   table_name, num_contacts, key, family_id);
  
  ntop->getTrace()->traceEvent(TRACE_INFO, "%s", sql);

  l->lock(__FILE__, __LINE__);  
  rc = credis_rpush(redis, CONTACTS_TO_DUMP, sql);
  credis_ltrim(redis, CONTACTS_TO_DUMP, 0, MAX_NUM_QUEUED_CONTACTS);
  l->unlock(__FILE__, __LINE__);

 return(rc);
}

/* **************************************** */

int Redis::popContactToDump(char *buf, u_int buf_len) {
  char *val;
  int rc;

  l->lock(__FILE__, __LINE__);
  rc = credis_lpop(redis, (char*)CONTACTS_TO_DUMP, &val);
  l->unlock(__FILE__, __LINE__);

  if(rc == 0)
    snprintf(buf, buf_len, "%s", val);
  else
    buf[0] = '\0';

  return(rc);
}
