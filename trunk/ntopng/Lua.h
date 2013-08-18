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

#ifndef _LUA_H_
#define _LUA_H_

#include "ntop_includes.h"

/* ******************************* */

class Lua {
 private:
  lua_State *L;

  void lua_register_classes(lua_State *L, bool http_mode);

 public:
  Lua();
  ~Lua();

  int run_script(char *script_path, char *ifname);
  int handle_script_request(struct mg_connection *conn, const struct mg_request_info *request_info, char *script_path);
};

extern void lua_push_str_table_entry(lua_State *L, const char *key, char *value);
extern void lua_push_nil_table_entry(lua_State *L, const char *key);
extern void lua_push_int_table_entry(lua_State *L, const char *key, u_int64_t value);
extern void lua_push_bool_table_entry(lua_State *L, const char *key, bool value);
extern void lua_push_float_table_entry(lua_State *L, const char *key, float value);

#endif /* _LUA_H_ */
