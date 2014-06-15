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

#ifndef _IP_ADDRESS_H_
#define _IP_ADDRESS_H_

#include "ntop_includes.h"

struct ipAddress {
  u_int8_t ipVersion:3 /* Either 4 or 6 */, 
    localHost:1, privateIP:1,
    notUsed:4 /* Future use */;

  union {
    struct ndpi_in6_addr ipv6;
    u_int32_t ipv4; /* Host byte code */
  } ipType;
};

/* *********************************** */

class IpAddress {
 private:
  struct ipAddress addr;
  u_int32_t ip_key;

  char* _intoaV4(unsigned int addr, char* buf, u_short bufLen);
  char* _intoa(char* buf, u_short bufLen);
  void checkPrivate();
  void compute_key();
  
 public:
  IpAddress();
  IpAddress(char *string);
  IpAddress(IpAddress *ip);
  IpAddress(u_int32_t _ipv4);
  IpAddress(struct ndpi_in6_addr *_ipv6);

  bool isEmpty();
  inline void reset()                                 { memset(&addr, 0, sizeof(addr));               }
  inline bool isIPv4()                                { return((addr.ipVersion == 4) ? true : false); }
  inline void set_ipv4(u_int32_t _ipv4)               { addr.ipVersion = 4, addr.ipType.ipv4 = _ipv4; compute_key(); }
  inline void set_ipv6(struct ndpi_in6_addr *_ipv6)   { addr.ipVersion = 6, memcpy(&addr.ipType.ipv6, _ipv6, sizeof(struct ndpi_in6_addr)); compute_key(); }
  inline u_int32_t get_ipv4()                         { return((addr.ipVersion == 4) ? addr.ipType.ipv4 : 0);     }
  inline struct ndpi_in6_addr* get_ipv6()             { return((addr.ipVersion == 6) ? &addr.ipType.ipv6 : NULL); }
  inline struct ipAddress* getIP()                    { return(&addr); };
  inline bool equal(u_int32_t ipv4_addr)              { if((addr.ipVersion == 4) && (addr.ipType.ipv4 == ipv4_addr)) return(true); else return(false); };
  inline bool equal(struct ndpi_in6_addr *ip6_addr)   { if((addr.ipVersion == 6) && (memcmp(&addr.ipType.ipv6, ip6_addr, sizeof(struct ndpi_in6_addr)) == 0)) return(true); else return(false); };
  inline bool equal(IpAddress *_ip)                   { return(this->compare(_ip) == 0); };

  void set_from_string(char *string);
  int compare(IpAddress *ip);
  inline u_int32_t key()                               { return(ip_key);         };
  void set(IpAddress *ip);
  void dump(struct sockaddr *sa);
  inline bool isPrivateAddress()                       { return(addr.privateIP); };
  char* print(char *str, u_int str_len);
  bool isLocalHost(int16_t *network_id);
  bool isLocalInterfaceAddress();
  void deserialize(json_object *o);
  char* serialize();
  json_object* getJSONObject();
};

#endif /* _IP_ADDRESS_H_ */
