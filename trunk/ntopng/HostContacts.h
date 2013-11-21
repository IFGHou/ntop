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

#ifndef _HOST_CONTACTS_H_
#define _HOST_CONTACTS_H_

#include "ntop_includes.h"

typedef struct {
  IpAddress *host;
  u_int32_t num_contacts;
} IPContacts;

class GenericHost;

class HostContacts {
 protected:
  IPContacts clientContacts[MAX_NUM_HOST_CONTACTS], serverContacts[MAX_NUM_HOST_CONTACTS];  
  void incrIPContacts(NetworkInterface *iface, IpAddress *ip, IpAddress *peer, 
		      IPContacts *contacts, u_int32_t value, bool aggregated_host);

 public:
  HostContacts();
  ~HostContacts();

  void dbDump(char *daybuf, char *ifname, char *key, u_int16_t family_id);
  inline void incrContact(NetworkInterface *iface, IpAddress *me, 
			  IpAddress *peer, bool contacted_peer_as_client,
			  u_int32_t value, bool aggregated_host) { 
    incrIPContacts(iface, me, peer, contacted_peer_as_client ? clientContacts : serverContacts, value, aggregated_host);
  };

  u_int get_num_contacts_by(IpAddress* host_ip);
  bool dumpHostToDB(IpAddress *host, LocationPolicy policy);
  void getIPContacts(lua_State* vm);
  char* serialize();
  void deserialize(NetworkInterface *iface, GenericHost *h, json_object *o);
  json_object* getJSONObject();
};

#endif /* _HOST_CONTACTS_H_ */
