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

#ifndef _NDPI_STATS_H_
#define _NDPI_STATS_H_

#include "ntop_includes.h"

#define MAX_NDPI_PROTOS       (NDPI_MAX_SUPPORTED_PROTOCOLS + NDPI_MAX_NUM_CUSTOM_PROTOCOLS + 1)

/* *************************************** */

typedef struct {
  u_int64_t sent, rcvd;
} TrafficCounter;

class NetworkInterface;

/* *************************************** */

class NdpiStats : public Serializable {
 private:
  TrafficCounter packets[MAX_NDPI_PROTOS], bytes[MAX_NDPI_PROTOS];

 public:
  NdpiStats();

  void sumStats(NdpiStats *stats);

  inline void incStats(u_int proto_id,
		       u_int64_t sent_packets, u_int64_t sent_bytes,
		       u_int64_t rcvd_packets, u_int64_t rcvd_bytes) {
    if(proto_id < (MAX_NDPI_PROTOS)) {
      packets[proto_id].sent += sent_packets, bytes[proto_id].sent += sent_bytes;
      packets[proto_id].rcvd += rcvd_packets, bytes[proto_id].rcvd += rcvd_bytes;
    }
  };

  inline TrafficCounter* getPackets(u_int16_t proto_id) { if(proto_id < (MAX_NDPI_PROTOS)) return(&packets[proto_id]); else return(NULL); };
  inline TrafficCounter* getBytes(u_int16_t proto_id)   { if(proto_id < (MAX_NDPI_PROTOS)) return(&bytes[proto_id]);   else return(NULL); };
  void print(NetworkInterface *iface);
  void lua(NetworkInterface *iface, lua_State* vm);

  const char* serialize();
  void  deserialize(const char*);
};

#endif /* _NDPI_STATS_H_ */
