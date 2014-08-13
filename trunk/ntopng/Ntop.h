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

#ifndef _NTOP_CLASS_H_
#define _NTOP_CLASS_H_

#include "ntop_includes.h"

/** @defgroup Ntop Ntop
 * Main ntopng group.
 */


/** @class Ntop
 *  @brief Main class of ntopng.
 *
 *  @ingroup Ntop
 *
 */
class Ntop {
 private:
  patricia_tree_t *local_interface_addresses;
  char working_dir[MAX_PATH]; /**< Array of working directory.*/
  char install_dir[MAX_PATH]; /**< Array of install directory.*/
  char startup_dir[MAX_PATH]; /**< Array of startup directory.*/
  char *custom_ndpi_protos; /**< Pointer of a custom protocol for nDPI.*/
  NetworkInterface *iface[MAX_NUM_DEFINED_INTERFACES];/**< Array of network interfaces.*/
  u_int8_t num_defined_interfaces; /**< Number of defined interfaces.*/
  HTTPserver *httpd; /**< Pointer of httpd server.*/
  NtopGlobals *globals; /**< Pointer of Ntop globals info and variables.*/
  Redis *redis; /**< Pointer of Radius server.*/
  PeriodicActivities *pa; /**< Instance of periodical activities.*/
  AddressResolution *address;
  Prefs *prefs;
  RuntimePrefs *runtimeprefs;
  Geolocation *geo;
  Categorization *categorization;
  HTTPBL* httpbl;
  ExportInterface *export_interface;
  int8_t historical_interface_id;
  Mutex *rrd_lock;
  long time_offset;
  time_t start_time; /**< Time when start() was called */

  void loadLocalInterfaceAddress();

 public:
  /**
   * @brief A Constructor
   * @details Creating a new Ntop.
   *
   * @param appName  Describe the application name.
   * @return A new instance of Ntop.
   */
  Ntop(char *appName);
  /**
   * @brief A Destructor.
   *
   */
  ~Ntop();
  /**
   * @brief Register the ntopng preferences.
   * @details Setting the ntopng preferences defined in a Prefs instance.
   *
   * @param _prefs Prefs instance containing the ntopng preferences.
   */
  void registerPrefs(Prefs *_prefs);
  /**
   * @brief Set the path of custom nDPI protocols file.
   * @details Set the path of protos.txt containing the defined custom protocols. For more information please read the nDPI quick start (cd ntopng source code directory/nDPI/doc/).
   *
   * @param path Path of protos.file.
   */
  void setCustomnDPIProtos(char *path);
  /**
   * @brief Get the custom nDPI protocols.
   * @details Inline function.
   *
   * @return The path of custom nDPI protocols file.
   */
  inline char* getCustomnDPIProtos()                 { return(custom_ndpi_protos);                 };
  /**
   * @brief Get the offset time.
   * @details ....
   *
   * @return The timezone offset.
   */
  inline long get_time_offset()                      { return(time_offset);                        };
  /**
   * @brief Initialize the Timezone.
   * @details Use the localtime function to initialize the variable @ref time_offset.
   *
   */
  void initTimezone();
  /**
   * @brief Get a valid path.
   * @details Processes the input path and return a valid path.
   *
   * @param path String path to validate.
   * @return A valid path.
   */
  char* getValidPath(char *path);
  /**
   * @brief Load the @ref Geolocation module.
   * @details Initialize the variable @ref geo with the input directory.
   *
   * @param dir Path to database home directory.
   */
  void loadGeolocation(char *dir);
  /**
   * @brief Set the local networks.
   * @details Set the local networks to @ref AddressResolution instance.
   *
   * @param nets String that defined the local network with this Format: 131.114.21.0/24,10.0.0.0/255.0.0.0 .
   */
  void setLocalNetworks(char *nets);
  /**
   * @brief Check if the ingress parameter is in the local networks.
   * @details Inline method.
   *
   * @param family Internetwork: UDP, TCP, etc.
   * @param addr Internet Address.
   * @param network_id It returns the networkId to which the host belongs to
   * @return True if the address is in the local networks, false otherwise.
   */
  bool isLocalAddress(int family, void *addr, int16_t *network_id);

  /**
   * @brief Start ntopng packet processing.
   */
  void start();
  /**
   * @brief Resolve the host name.
   * @details Use the redis database to resolve the IP address and get the host name.
   *
   * @param numeric_ip Address IP.
   * @param symbolic Symbolic name.
   * @param symbolic_len Length of symbolic name.
   */
  inline void resolveHostName(char *numeric_ip, char *symbolic, u_int symbolic_len) {
    address->resolveHostName(numeric_ip, symbolic, symbolic_len);
  }
  /**
   * @brief Get the geolocation instance.
   *
   * @return Current geolocation instance.
   */
  inline Geolocation* getGeolocation()               { return(geo);                        };
  /**
   * @brief Get the ifName.
   * @details Find the ifName by id parameter.
   *
   * @param id Index of ifName.
   * @return ....
   */
  inline char* get_if_name(u_int8_t id)              { return(prefs->get_if_name(id));     };
  inline char* get_if_descr(u_int8_t id)             { return(prefs->get_if_descr(id));    };
  inline char* get_data_dir()                        { return(prefs->get_data_dir());      };
  inline char* get_callbacks_dir()                   { return(prefs->get_callbacks_dir()); };
  /**
   * @brief Get categorization.
   *
   * @return Current categorization instance.
   */
  inline Categorization* get_categorization()        { return(categorization);             };
  /**
   * @brief Get httpbl.
   *
   * @return Current httpbl instance.
   */
  inline HTTPBL* get_httpbl()                        { return(httpbl);             };

  /**
   * @brief Register the network interface.
   * @details Check for duplicated interface and add the network interface in to @ref iface.
   *
   * @param i Network interface.
   */
  void registerInterface(NetworkInterface *i);
  /**
   * @brief Get the number of defined network interfaces.
   *
   * @return Number of defined network interfaces.
   */
  inline u_int8_t get_num_interfaces()               { return(num_defined_interfaces); }
  /**
   * @brief Get the network interface identified by Id.
   *
   * @param i Index of network interface.
   * @return The network interface instance if exists, NULL otherwise.
   */
  NetworkInterface* getInterfaceById(u_int8_t i);
  /**
   * @brief Get the i-th network interface.
   *
   * @param i The i-th network interface.
   * @return The network interface instance if exists, NULL otherwise.
   */
  NetworkInterface* getInterfaceAtId(u_int8_t i) { if(i<num_defined_interfaces) return(iface[i]); else return(NULL); }
  /**
   * @brief Get the network interface identified by name or Id.
   * @details This method accepts both interface names or Ids.
   *
   * @param name Name of network interface.
   * @return The network interface instance if exists, NULL otherwise.
   */
  NetworkInterface* getInterface(char *name);
  /**
   * @brief Get the Id of network interface.
   * @details This method accepts both interface names or Ids.
   *
   * @param name Name of network interface.
   * @return The network interface Id if exists, -1 otherwise.
   */
  int getInterfaceIdByName(char *name);
  /**
   * @brief Register the HTTP server.
   *
   * @param h HTTP server instance.
   */
  inline void registerHTTPserver(HTTPserver *h)      { httpd = h;              };
  /**
   * @brief Set categorization.
   *
   * @param c The categorization instance.
   */
  inline void setCategorization(Categorization *c)   { categorization = c; };
  /**
   * @brief Set httpbl.
   *
   * @param h The categorization instance.
   */
  inline void setHTTPBL(HTTPBL *h)                   { httpbl = h; };

  /**
   * @brief Get the network interface identified by name or Id.
   * @details This method accepts both interface names or Ids.
   *
   * @param name Names or Id of network interface.
   * @return The network interface instance if exists, NULL otherwise.
   */
  NetworkInterface* getNetworkInterface(const char *name);
  /**
   * @brief Get the current HTTPserver instance.
   *
   * @return The current instance of HTTP server.
   */
  inline HTTPserver*       get_HTTPserver()          { return(httpd);            };
  /**
   * @brief Get the current working directory.
   *
   * @return The absolute path of working directory.
   */
  inline char* get_working_dir()                     { return(working_dir);      };
  /**
   * @brief Get the installation path of ntopng.
   *
   * @return The path of installed directory.
   */
  inline char* get_install_dir()                     { return(install_dir);      };

  inline NtopGlobals*      getGlobals()              { return(globals); };
  inline Trace*            getTrace()                { return(globals->getTrace()); };
  inline Redis*            getRedis()                { return(redis);               };
  inline Prefs*            getPrefs()                { return(prefs);               };
  inline RuntimePrefs*            getRuntimePrefs()                { return(runtimeprefs);               };

  inline void rrdLock(const char *filename, const int line)   { rrd_lock->lock(filename, line);   };
  inline void rrdUnlock(const char *filename, const int line) { rrd_lock->unlock(filename, line); };
  inline ExportInterface* get_export_interface()              { return(export_interface);         };

  void getUsers(lua_State* vm);
  int  checkUserPassword(const char *user, const char *password);
  int  resetUserPassword(char *username, char *old_password, char *new_password);
  int  addUser(char *username, char *full_name, char *password);
  int  deleteUser(char *username);
  void setWorkingDir(char *dir);
  void fixPath(char *str);
  void removeTrailingSlash(char *str);
  void daemonize();
  void shutdown();
  void runHousekeepingTasks();
  bool isLocalInterfaceAddress(int family, void *addr);
  inline char* getLocalNetworkName(int16_t local_network_id) { return(address->get_local_network((u_int8_t)local_network_id)); };
  void createExportInterface();

// Historical Interface
/**
 * @brief Get Historical Interface
 * @details Use  @ref historical_interface_id to identify the Historical Interface
 * @return Pointer to Historical Interface instance
 */
  NetworkInterface* getHistoricalInterface();
  /**
   * @brief Get Historical Interface index
   * @return index of current historical interface instance
   */
  inline u_int8_t getHistoricalInterfaceId() {  return historical_interface_id; };
  /**
   * @brief Create a new Historical Interface instance
   * @details Create a new instance and register the interface
   */
  void createHistoricalInterface();

  inline u_int32_t getUptime() { return((u_int32_t)((start_time > 0) ? (time(NULL)-start_time) : 0)); }
};

extern Ntop *ntop;

#endif /* _NTOP_CLASS_H_ */
