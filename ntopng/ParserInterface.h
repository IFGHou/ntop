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

#ifndef _PARSER_INTERFACE_H_
#define _PARSER_INTERFACE_H_

#include "ntop_includes.h"

class ParserInterface : public NetworkInterface {
 private:

 public:
  ParserInterface(const char *endpoint);
  static u_int8_t parse_flows(char *payload, int payload_size, u_int8_t source_id, void *data);
};

#endif /* _PARSER_INTERFACE_H_ */


