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

#include <sys/stat.h>

extern "C" {
#include <microhttpd.h>
#undef _GNU_SOURCE
#undef _POSIX_C_SOURCE
#undef _XOPEN_SOURCE

#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
};

static HTTPserver *httpserver;

#define PAGE_NOT_FOUND "<html><head><title>ntop</title></head><body>Page &quot;%s&quot; was not found</body></html>"
#define PAGE_ERROR     "<html><head><title>ntop</title></head><body>Script &quot;%s&quot; returned an error:<p>\n<pre>%s</pre></body></html>"

/* ****************************************** */

static int page_not_found(struct MHD_Connection *connection, const char *url) {
  char rsp[4096];

  snprintf(rsp, sizeof(rsp), PAGE_NOT_FOUND, url);

  struct MHD_Response *response = MHD_create_response_from_buffer(strlen(rsp), (void *)rsp, MHD_RESPMEM_PERSISTENT);
  int ret = MHD_queue_response(connection, MHD_HTTP_NOT_FOUND, response);
  MHD_destroy_response(response);

  ntop->getTrace()->traceEvent(TRACE_WARNING, "[HTTP] Page not found %s", url);
  return(ret);
}

/* ****************************************** */

static int page_error(struct MHD_Connection *connection, const char *url, const char *err) {
  char rsp[4096];

  snprintf(rsp, sizeof(rsp), PAGE_ERROR, url, err);

  struct MHD_Response *response = MHD_create_response_from_buffer(strlen(rsp), (void *)rsp, MHD_RESPMEM_PERSISTENT);
  int ret = MHD_queue_response(connection, MHD_HTTP_NOT_FOUND, response);
  MHD_destroy_response(response);

  ntop->getTrace()->traceEvent(TRACE_WARNING, "[HTTP] Script error %s", url);
  return(ret);
}

/* ****************************************** */

static ssize_t file_reader(void *cls, uint64_t pos, char *buf, size_t max) {
  FILE *file = (FILE*)cls;
  (void)fseek(file, pos, SEEK_SET);
  return(fread(buf, 1, max, file));
}

/* ****************************************** */

static void free_callback(void *cls) {
  fclose((FILE*)cls);
}

/* *********************************************************** */

static int ntop_lua_check(lua_State* vm, const char* func,
			  int pos, int expected_type) {
  if(lua_type(vm, pos) != expected_type) {
    ntop->getTrace()->traceEvent(TRACE_ERROR,
					"%s : expected %s, got %s", func,
					lua_typename(vm, expected_type),
					lua_typename(vm, lua_type(vm,pos)));
    return(-1);
  }

  return(0);
}

/* ****************************************** */

static int ntop_dump_file(lua_State* vm) {
  char *fname, tmp[1024];
  FILE *f;
  FILE *tmp_file;

  lua_getglobal(vm, "tmp_file");
  if((tmp_file = (FILE*)lua_touserdata(vm, lua_gettop(vm))) == NULL) {
    ntop->getTrace()->traceEvent(TRACE_ERROR, "INTERNAL ERROR: null file");
    return(0);
  }

  if(ntop_lua_check(vm, __FUNCTION__, 1, LUA_TSTRING)) return(0);
  if((fname = (char*)lua_tostring(vm, 1)) == NULL)     return(-1);

  if((f = fopen(fname, "r")) == NULL) {
    ntop->getTrace()->traceEvent(TRACE_ERROR, "Unable to open file %s", fname);
    return(1);
  }
  
  while((fgets(tmp, sizeof(tmp), f)) != NULL)
    fprintf(tmp_file, "%s", tmp);
 
  fclose(f);
 
  return(1);
}

/* ****************************************** */

static int ntop_find_interface(lua_State* vm) {
  char *ifname;

  if(ntop_lua_check(vm, __FUNCTION__, 1, LUA_TSTRING)) return(0);
  ifname = (char*)lua_tostring(vm, 1);

  lua_pushlightuserdata(vm, (char*)ntop->get_NetworkInterface(ifname));
  lua_setglobal(vm, "ntop_interface");
 
  return(1);
}

/* ****************************************** */

static void lua_push_str_table_entry(lua_State *L, const char *key, char *value) {
  lua_pushstring(L, key);
  lua_pushstring(L, value);
  lua_settable(L, -3);
}

/* ****************************************** */

static void lua_push_int_table_entry(lua_State *L, const char *key, int value) {
  lua_pushstring(L, key);
  lua_pushinteger(L, value);
  lua_settable(L, -3);
}

/* ****************************************** */

static int ntop_get_interface_stats(lua_State* vm) {
  NetworkInterface *ntop_interface;
  InterfaceStats *stats;
  
  lua_getglobal(vm, "ntop_interface");
  if((ntop_interface = (NetworkInterface*)lua_touserdata(vm, lua_gettop(vm))) == NULL) {
    ntop->getTrace()->traceEvent(TRACE_ERROR, "INTERNAL ERROR: null interface");
    return(0);
  } else
    stats = ntop_interface->getStats();

  lua_newtable(vm);
  lua_push_int_table_entry(vm, "packets", stats->getNumPkts());
  lua_push_int_table_entry(vm, "bytes", stats->getNumBytes());
 
  return(1);
}

/* ****************************************** */

static int ntop_lua_print(lua_State* vm) {
  FILE *tmp_file;

  lua_getglobal(vm, "tmp_file");
  if((tmp_file = (FILE*)lua_touserdata(vm, lua_gettop(vm))) == NULL) {
    ntop->getTrace()->traceEvent(TRACE_ERROR, "INTERNAL ERROR: null file");
    return(0);
  }

  switch(lua_type(vm, 1)) {
  case LUA_TSTRING:
    {
      char *str = (char*)lua_tostring(vm, 1);
      if(str && (strlen(str) > 0))
	fprintf(tmp_file, "%s", str);
    }
    break;

  case LUA_TNUMBER:
    fprintf(tmp_file, "%f", (float)lua_tonumber(vm, 1));
    break;

  default:
    return(0);
  }

  return(1);
}

/* ****************************************** */

typedef struct {
  const char *class_name;
  const luaL_Reg *class_methods;
} ntop_class_reg;

static const luaL_Reg ntop_interface_reg[] = {
  { "find",           ntop_find_interface },
  { "getStats",       ntop_get_interface_stats },
  {NULL,           NULL}
};

static const luaL_Reg ntop_reg[] = {
  { "dumpFile",           ntop_dump_file },
  {NULL,           NULL}
};

/* ****************************************** */

static void lua_register_classes(lua_State *L) {
  int lib_id, meta_id;
  static const luaL_Reg _meta[] = { { NULL, NULL } };                           
  int i;
  ntop_class_reg ntop[] = {
    { "interface", ntop_interface_reg },    
    { "ntop",     ntop_reg },    
    {NULL,    NULL}
  };
  
  for(i=0; ntop[i].class_name != NULL; i++) {
    /* newclass = {} */
    lua_createtable(L, 0, 0);
    lib_id = lua_gettop(L);
    
    /* metatable = {} */
    luaL_newmetatable(L, ntop[i].class_name);
    meta_id = lua_gettop(L);
    luaL_register(L, NULL, _meta);

    /* metatable.__index = class_methods */
    lua_newtable(L), luaL_register(L, NULL, ntop[i].class_methods);
    lua_setfield(L, meta_id, "__index");    
    
    /* metatable.__metatable = _meta */
    //luaL_newlib(L, _meta);
    //lua_setfield(L, meta_id, "__metatable");
    
    /* class.__metatable = metatable */
    lua_setmetatable(L, lib_id);
    
    /* _G["Foo"] = newclass */
    lua_setglobal(L, ntop[i].class_name);
  }
}

/* ****************************************** */

int MHD_KeyValueIteratorGet(void *cls, enum MHD_ValueKind kind,
			    const char *key, const char *value) {
  lua_State *L = (lua_State*)cls;

  lua_push_str_table_entry(L, key, (char*)value);
  return(MHD_YES);
}

/* ****************************************** */

static int handle_script_request(char *script_path,
				 void *cls,
				 struct MHD_Connection *connection,
				 const char *url,
				 const char *method,
				 const char *version,
				 const char *upload_data,
				 size_t *upload_data_size, void **ptr) {
  int ret = 0;
  lua_State *L = luaL_newstate();
  FILE *tmp_file = NULL;
  MHD_Response *tmp_response;
  
  if(L == NULL) {
    ntop->getTrace()->traceEvent(TRACE_ERROR, "[HTTP] Unable to start LUA interpreter");
    return(page_not_found(connection, url));
  } else {
    /* Load base libraries */
    luaL_openlibs(L);

    /* Load custom classes */
    lua_register_classes(L);

    /* Register the connection in the state */
    tmp_file = tmpfile();
    lua_pushlightuserdata(L, (char*)tmp_file);
    lua_setglobal(L, "tmp_file");

    /* Put the GET params into the environment */
    lua_newtable(L);
    MHD_get_connection_values(connection, MHD_GET_ARGUMENT_KIND, MHD_KeyValueIteratorGet, (void*)L);
    lua_setglobal(L, "_GET"); /* Like in php */

    /* Overload the standard Lua print() with ntop_lua_print that dumps data on HTTP server */
    lua_register(L, "print", ntop_lua_print);

    tmp_response = MHD_create_response_from_fd(MHD_SIZE_UNKNOWN, fileno(tmp_file));
  }

  if(luaL_dofile(L, script_path) == 0) {
    fflush(tmp_file);
    /* Don't call fclose(tnmp_file) as the file is closed automatically by the httpd */
    ret = MHD_queue_response(connection, MHD_HTTP_OK, tmp_response);    
    MHD_destroy_response(tmp_response);
  } else {
    ret = page_error(connection, url, lua_tostring(L, -1));    
  }

  lua_close(L);

  return(ret);
}

/* ****************************************** */

static int handle_http_request(void *cls,
			       struct MHD_Connection *connection,
			       const char *url,
			       const char *method,
			       const char *version,
			       const char *upload_data,
			       size_t *upload_data_size, void **ptr) {
  static int aptr;
  struct MHD_Response *response;
  int ret;
  FILE *file;
  struct stat buf;
  char path[255] = { 0 };

  if(0 != strcmp(method, MHD_HTTP_METHOD_GET))
    return MHD_NO;              /* unexpected method */

  if(&aptr != *ptr) {
    /* do never respond on first call */
    *ptr = &aptr;
    return MHD_YES;
  }

  *ptr = NULL;                  /* reset when done */

  /* 1 - check if this is a static file */
  snprintf(path, sizeof(path), "%s%s", httpserver->get_docs_dir(),
	   (strlen(url) == 1) ? "/index.html" : url);

  if((stat(path, &buf) == 0) && (S_ISREG (buf.st_mode)))
    file = fopen(path, "rb");
  else
    file = NULL;

  if(file == NULL) {
    /* 2 - check if this a script file */
    snprintf(path, sizeof(path), "%s%s", httpserver->get_scripts_dir(),
	     (strlen(url) == 1) ? "/index.lua" : url);

    if((stat(path, &buf) == 0) && (S_ISREG (buf.st_mode))) {
      ntop->getTrace()->traceEvent(TRACE_NORMAL, "[HTTP] %s [%s]", url, path);
      ret = handle_script_request(path, cls, connection, url, method, version, upload_data, upload_data_size, ptr);
    } else {
      ret = page_not_found(connection, url);
    }
  } else {
    ntop->getTrace()->traceEvent(TRACE_NORMAL, "[HTTP] %s [%s]", url, path);

    response = MHD_create_response_from_callback (buf.st_size, 32 * 1024,     /* 32k page size */
						  &file_reader,
						  file,
						  &free_callback);
    if(response == NULL) {
      fclose(file);
      return MHD_NO;
    }

    ret = MHD_queue_response (connection, MHD_HTTP_OK, response);
    MHD_destroy_response(response);
    /* fclose(file) is not necessary as the HTTP library does it automatically */
  }

  return ret;
}

/* ****************************************** */

HTTPserver::HTTPserver(u_int16_t _port, const char *_docs_dir, const char *_scripts_dir) {
  port = _port, docs_dir = strdup(_docs_dir), scripts_dir = strdup(_scripts_dir);

  httpd_v4 = MHD_start_daemon(MHD_USE_SELECT_INTERNALLY | MHD_USE_DEBUG,
			   port, NULL, NULL, &handle_http_request, (void*)PAGE_NOT_FOUND,
			   MHD_OPTION_CONNECTION_TIMEOUT, (unsigned int)120,
			   MHD_OPTION_END);

  if(httpd_v4 == NULL) {
    ntop->getTrace()->traceEvent(TRACE_ERROR, "Unable to start HTTP server (IPv4) on port %d", port);
    exit(-1);
  }

  /* ***************************** */
  
  httpd_v6 = MHD_start_daemon(MHD_USE_SELECT_INTERNALLY | MHD_USE_DEBUG | MHD_USE_IPv6,
			      port, NULL, NULL, &handle_http_request, (void*)PAGE_NOT_FOUND,
			      MHD_OPTION_CONNECTION_TIMEOUT, (unsigned int)120,
			      MHD_OPTION_END);

  if(httpd_v6 == NULL) {
    ntop->getTrace()->traceEvent(TRACE_ERROR, "Unable to start HTTP server (IPv6) on port %d", port);
  }

  /* ***************************** */

  httpserver = this;
  ntop->getTrace()->traceEvent(TRACE_NORMAL, "HTTP server listening on port %d [%s][%s]",
				      port, docs_dir, scripts_dir);
};

/* ****************************************** */

HTTPserver::~HTTPserver() {
  if(httpd_v4) MHD_stop_daemon(httpd_v4);
  if(httpd_v6) MHD_stop_daemon(httpd_v6);

  ntop->getTrace()->traceEvent(TRACE_NORMAL, "HTTP server terminated");
};

/* ****************************************** */


