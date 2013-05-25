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

#ifdef HAVE_PF_RING

#ifndef _PF_RING_NETWORK_INTERFACE_H_
#define _PF_RING_NETWORK_INTERFACE_H_

#include "ntop_includes.h"

class PF_RINGInterface : public NetworkInterface {
 private:
  pfring *pfring_handle;

 public:
  PF_RINGInterface(char *name, bool change_user);
  ~PF_RINGInterface();

  inline pfring* get_pfring_handle() { return(pfring_handle); };
  void startPacketPolling();
  void shutdown();
};

#endif /* _PF_RING_NETWORK_INTERFACE_H_ */

#endif /* HAVE_PF_RING */

