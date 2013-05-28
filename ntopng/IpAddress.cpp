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

/* ******************************************* */

IpAddress::IpAddress() {
  ;
}

/* ******************************************* */

IpAddress::IpAddress(char *string) {
  set_from_string(string);
}

/* ******************************************* */

IpAddress::IpAddress(IpAddress *ip) {
  memcpy(&addr, &ip->addr, sizeof(struct ipAddress));
}

/* ******************************************* */

IpAddress::IpAddress(u_int32_t _ipv4) {
  set_ipv4(_ipv4);
}

/* ******************************************* */

IpAddress::IpAddress(struct ndpi_in6_addr *_ipv6) {
  set_ipv6(_ipv6);
}

/* ******************************************* */

void IpAddress::set_from_string(char *string) {
  if(strchr(string, '.')) {
    addr.ipVersion = 4, addr.localHost = 0, addr.ipType.ipv4 = inet_addr(string);
  } else {
    if(inet_pton(AF_INET6, string, &addr.ipType.ipv6) <= 0) {
      /* We failed */
      addr.ipVersion = 4, addr.localHost = 0, addr.ipType.ipv4 = 0;
    } else {
      addr.ipVersion = 6, addr.localHost = 0;
    }
  }
}

/* ******************************************* */

int IpAddress::compare(IpAddress *ip) {
  if(addr.ipVersion < ip->addr.ipVersion) return(-1); else if(addr.ipVersion > ip->addr.ipVersion) return(1);

  if(addr.ipVersion == 4)
    return(memcmp(&addr.ipType.ipv4, &ip->addr.ipType.ipv4, sizeof(u_int32_t)));
  else
    return(memcmp(&addr.ipType.ipv6, &ip->addr.ipType.ipv6, sizeof(struct ndpi_in6_addr)));
}

/* ******************************************* */

u_int32_t IpAddress::key() {
  if(addr.ipVersion == 4)
    return(addr.ipType.ipv4);
  else {
    u_int32_t key=0;

    for(u_int32_t i=0; i<4; i++)
      key += addr.ipType.ipv6.__u6_addr.__u6_addr32[i];

    return(key);
  }
}

/* ******************************************* */

char* IpAddress::_intoaV4(unsigned int addr, char* buf, u_short bufLen) {
  char *cp, *retStr;
  uint byte;
  int n;

  cp = &buf[bufLen];
  *--cp = '\0';

  n = 4;
  do {
    byte = addr & 0xff;
    *--cp = byte % 10 + '0';
    byte /= 10;
    if(byte > 0) {
      *--cp = byte % 10 + '0';
      byte /= 10;
      if(byte > 0)
	*--cp = byte + '0';
    }
    *--cp = '.';
    addr >>= 8;
  } while (--n > 0);

  /* Convert the string to lowercase */
  retStr = (char*)(cp+1);

  return(retStr);
}

/* ****************************** */

char* IpAddress::_intoa(char* buf, u_short bufLen) {
  if((addr.ipVersion == 4) || (addr.ipVersion == 0 /* Misconfigured */))
    return(_intoaV4(ntohl(addr.ipType.ipv4), buf, bufLen));
  else {
    char *ret = (char*)inet_ntop(AF_INET6, &addr.ipType.ipv6, buf, bufLen);

    if(ret == NULL) {
      /* Internal error (buffer too short) */
      buf[0] = '\0';
      return(buf);
    } else
      return(ret);
  }
}

/* ******************************************* */

char* IpAddress::print(char *str, u_int str_len) {
  return(_intoa(str, str_len));
}

/* ******************************************* */

void IpAddress::dump(struct sockaddr *sa) {
  memset(sa, 0, sizeof(struct sockaddr));

  if(addr.ipVersion == 4) {
    struct sockaddr_in *in4 = (struct sockaddr_in*)sa;

    in4->sin_family = AF_INET, in4->sin_addr.s_addr = addr.ipType.ipv4;
  } else {
    struct sockaddr_in6 *in6 = (struct sockaddr_in6*)sa;

    memcpy(&in6->sin6_addr, &addr.ipType.ipv6, sizeof(struct ndpi_in6_addr));
    in6->sin6_family = AF_INET6;
  }
}

/* ******************************************* */

bool IpAddress::isLocalHost() {
  if(addr.ipVersion == 4) {
    u_int32_t v = /* htonl */(addr.ipType.ipv4);

    return(ntop->isLocalAddress(AF_INET, (void*)&v));
  } else {
    return(ntop->isLocalAddress(AF_INET6, (void*)&addr.ipType.ipv6));
  }
}
