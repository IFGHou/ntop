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

DnsStats::DnsStats() {
  memset(&sent, 0, sizeof(struct dns_stats));
  memset(&rcvd, 0, sizeof(struct dns_stats));
}

/* *************************************** */

void DnsStats::luaStats(lua_State *vm, struct dns_stats *stats, const char *label) {
  lua_newtable(vm);

  lua_push_int_table_entry(vm, "num_queries", stats->num_queries);
  lua_push_int_table_entry(vm, "num_replies_ok", stats->num_replies_ok);
  lua_push_int_table_entry(vm, "num_replies_error", stats->num_replies_error);
  lua_push_int_table_entry(vm, "num_a", stats->breakdown.num_a);
  lua_push_int_table_entry(vm, "num_ns", stats->breakdown.num_ns);
  lua_push_int_table_entry(vm, "num_cname", stats->breakdown.num_cname);
  lua_push_int_table_entry(vm, "num_soa", stats->breakdown.num_soa);
  lua_push_int_table_entry(vm, "num_ptr", stats->breakdown.num_ptr);
  lua_push_int_table_entry(vm, "num_mx", stats->breakdown.num_mx);
  lua_push_int_table_entry(vm, "num_txt", stats->breakdown.num_txt);
  lua_push_int_table_entry(vm, "num_aaaa", stats->breakdown.num_aaaa);
  lua_push_int_table_entry(vm, "num_any", stats->breakdown.num_any);
  lua_push_int_table_entry(vm, "num_other", stats->breakdown.num_other);
  lua_pushstring(vm, label);
  lua_insert(vm, -2);
  lua_settable(vm, -3);
}

/* *************************************** */

void DnsStats::lua(lua_State *vm) {
  lua_newtable(vm);

  luaStats(vm, &sent, "sent");
  luaStats(vm, &rcvd, "rcvd");

  lua_pushstring(vm, "dns");
  lua_insert(vm, -2);
  lua_settable(vm, -3);

}

/* *************************************** */

char* DnsStats::serialize() {
  json_object *my_object = getJSONObject();
  char *rsp = strdup(json_object_to_json_string(my_object));

  /* Free memory */
  json_object_put(my_object);

  return(rsp);
}

/* ******************************************* */

void DnsStats::deserializeStats(json_object *o, struct dns_stats *stats) {
  json_object *obj;

  if(json_object_object_get_ex(o, "num_queries", &obj)) stats->num_queries = json_object_get_int64(obj); else stats->num_queries = 0;
  if(json_object_object_get_ex(o, "num_replies_ok", &obj)) stats->num_replies_ok = json_object_get_int64(obj); else stats->num_replies_ok = 0;
  if(json_object_object_get_ex(o, "num_replies_error", &obj)) stats->num_replies_error = json_object_get_int64(obj); else stats->num_replies_error = 0;    
  if(json_object_object_get_ex(o, "num_a", &obj)) stats->breakdown.num_a = json_object_get_int64(obj); else stats->breakdown.num_a = 0;
  if(json_object_object_get_ex(o, "num_ns", &obj)) stats->breakdown.num_ns = json_object_get_int64(obj); else stats->breakdown.num_ns = 0;
  if(json_object_object_get_ex(o, "num_cname", &obj)) stats->breakdown.num_cname = json_object_get_int64(obj); else stats->breakdown.num_cname = 0;
  if(json_object_object_get_ex(o, "num_soa", &obj)) stats->breakdown.num_soa = json_object_get_int64(obj); else stats->breakdown.num_soa = 0;
  if(json_object_object_get_ex(o, "num_ptr", &obj)) stats->breakdown.num_ptr = json_object_get_int64(obj); else stats->breakdown.num_ptr = 0;
  if(json_object_object_get_ex(o, "num_mx", &obj)) stats->breakdown.num_mx = json_object_get_int64(obj); else stats->breakdown.num_mx = 0;
  if(json_object_object_get_ex(o, "num_txt", &obj)) stats->breakdown.num_txt = json_object_get_int64(obj); else stats->breakdown.num_txt = 0;
  if(json_object_object_get_ex(o, "num_aaaa", &obj)) stats->breakdown.num_aaaa = json_object_get_int64(obj); else stats->breakdown.num_aaaa = 0;
  if(json_object_object_get_ex(o, "num_any", &obj)) stats->breakdown.num_any = json_object_get_int64(obj); else stats->breakdown.num_any = 0;
  if(json_object_object_get_ex(o, "num_other", &obj)) stats->breakdown.num_other = json_object_get_int64(obj); else stats->breakdown.num_other = 0;
}

/* ******************************************* */

void DnsStats::deserialize(json_object *o) {
  json_object *obj;

  if(!o) return;

  if(json_object_object_get_ex(o, "sent", &obj))
    deserializeStats(obj, &sent);  

  if(json_object_object_get_ex(o, "rcvd", &obj))
    deserializeStats(obj, &sent);  
}

/* ******************************************* */

json_object* DnsStats::getStatsJSONObject(struct dns_stats *stats) {
  json_object *my_object = json_object_new_object();

  if(stats->num_queries > 0) json_object_object_add(my_object, "num_queries", json_object_new_int64(stats->num_queries));
  if(stats->num_replies_ok > 0) json_object_object_add(my_object, "num_replies_ok", json_object_new_int64(stats->num_replies_ok));
  if(stats->num_replies_error > 0) json_object_object_add(my_object, "num_replies_error", json_object_new_int64(stats->num_replies_error));

  if(stats->breakdown.num_a > 0) json_object_object_add(my_object, "num_a", json_object_new_int64(stats->breakdown.num_a));
  if(stats->breakdown.num_ns > 0) json_object_object_add(my_object, "num_ns", json_object_new_int64(stats->breakdown.num_ns));
  if(stats->breakdown.num_cname > 0) json_object_object_add(my_object, "num_cname", json_object_new_int64(stats->breakdown.num_cname));
  if(stats->breakdown.num_soa > 0) json_object_object_add(my_object, "num_soa", json_object_new_int64(stats->breakdown.num_soa));
  if(stats->breakdown.num_ptr > 0) json_object_object_add(my_object, "num_ptr", json_object_new_int64(stats->breakdown.num_ptr));
  if(stats->breakdown.num_mx > 0) json_object_object_add(my_object, "num_mx", json_object_new_int64(stats->breakdown.num_mx));
  if(stats->breakdown.num_txt > 0) json_object_object_add(my_object, "num_txt", json_object_new_int64(stats->breakdown.num_txt));
  if(stats->breakdown.num_aaaa > 0) json_object_object_add(my_object, "num_aaaa", json_object_new_int64(stats->breakdown.num_aaaa));
  if(stats->breakdown.num_any > 0) json_object_object_add(my_object, "num_any", json_object_new_int64(stats->breakdown.num_any));
  if(stats->breakdown.num_other > 0) json_object_object_add(my_object, "num_other", json_object_new_int64(stats->breakdown.num_other));
  
  return(my_object);
}

/* ******************************************* */

json_object* DnsStats::getJSONObject() {
  json_object *my_object = json_object_new_object();

  json_object_object_add(my_object, "sent", getStatsJSONObject(&sent));
  json_object_object_add(my_object, "rcvd", getStatsJSONObject(&rcvd));
  
  return(my_object);
}

/* ******************************************* */

void DnsStats::incQueryBreakdown(struct queries_breakdown *bd, u_int16_t query_type) {
  switch(query_type) {
  case 0:
    /* Zero means we have not been able to decode the DNS message */
    break;
  case 1:
    /* A */
    bd->num_a++;
    break;
  case 2:
    /* NS */
    bd->num_ns++;
    break;
  case 5: 
    /* CNAME */ 
    bd->num_cname++;
    break;
  case 6:
    /* SOA */ 
    bd->num_soa++;
    break;
  case 12:
    /* PTR */ 
    bd->num_ptr++;
    break;
  case 15:
    /* MX */
    bd->num_mx++;
    break;
  case 16:
    /* TXT */
    bd->num_txt++;
    break;
  case 28:
    /* AAAA */
    bd->num_aaaa++;
    break;
  case 255:
    /* ANY */ 
    bd->num_any++;
    break;
  default:
    
    bd->num_other++;
    break;
  }
}

/* ******************************************* */

void DnsStats::incNumDNSQueriesSent(u_int16_t query_type) {
  sent.num_queries++; 
  incQueryBreakdown(&sent.breakdown, query_type);
};

/* ******************************************* */

void DnsStats::incNumDNSQueriesRcvd(u_int16_t query_type /* IGNORED */) { 
  rcvd.num_queries++; 
  incQueryBreakdown(&rcvd.breakdown, query_type);
};
