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

#ifndef _COLLECTOR_INTERFACE_H_
#define _COLLECTOR_INTERFACE_H_

#include "ntop_includes.h"

class Lua;

class CollectorInterface : public NetworkInterface {
 private:
  char *endpoint, *topic;
  void *context, *subscriber;
  u_int32_t num_drops;
  
 public:
  CollectorInterface(const char *_endpoint, const char *_script_name);
  ~CollectorInterface();

  inline const char* get_type()         { return("zmq");      };
  inline bool is_ndpi_enabled()         { return(false);      };
  char *getEndpoint()                   { return endpoint;    };
  inline void incrDrops(u_int32_t num)  { num_drops += num;   };
  u_int getNumDroppedPackets()          { return(num_drops);  };
  void collect_flows();

  void startPacketPolling();
  void shutdown();

  bool set_packet_filter(char *filter);
};

#endif /* _COLLECTOR_INTERFACE_H_ */

