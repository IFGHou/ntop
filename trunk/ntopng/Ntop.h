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

#ifndef _NTOP_CLASS_H_
#define _NTOP_CLASS_H_

#include "ntop_includes.h"

/** @defgroup Ntop
 * Main ntopng group.
 */


/** @class Ntop
 *  @brief Main class of ntopng.
 *  @details .......
 *
 *  @ingroup nTop
 *
 */
 class Ntop {
 private:

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
   Geolocation *geo;
   Categorization *categorization;
   Mutex *rrd_lock;
   long time_offset;

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
   * @return True if the address is in the local networks, false otherwise.
   */
  inline bool isLocalAddress(int family, void *addr) { return(address->findAddress(family, addr)); };
  /**
   * @brief Star ntopng instance.
   */
  void start();
  /**
   * @brief Resolve the host name.
   * @details Use the redis database to resolve the host name.
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
  inline char* get_data_dir()                        { return(prefs->get_data_dir());      };
  inline char* get_callbacks_dir()                   { return(prefs->get_callbacks_dir()); };
  /**
   * @brief Get categorization.
   * 
   * @return Current categorization instance.
   */
  inline Categorization* get_categorization()        { return(categorization);             };
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
  inline NetworkInterface* getInterfaceId(u_int8_t i){ if(i<num_defined_interfaces) return(iface[i]); else return(NULL); }
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
   * @brief Get Net
   * @details [long description]
   * 
   * @param name [description]
   * @return [description]
   */
  NetworkInterface* getNetworkInterface(const char *name);
  inline HTTPserver*       get_HTTPserver()          { return(httpd);            };
  inline char* get_working_dir()                     { return(working_dir);      };
  inline char* get_install_dir()                     { return(install_dir);      };

  inline NtopGlobals*      getGlobals()              { return(globals); };
  inline Trace*            getTrace()                { return(globals->getTrace()); };
  inline Redis*            getRedis()                { return(redis);               };
  inline Prefs*            getPrefs()                { return(prefs);               };
  
  inline void rrdLock(const char *filename, const int line)   { rrd_lock->lock(filename, line);   };
  inline void rrdUnlock(const char *filename, const int line) { rrd_lock->unlock(filename, line); };

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
};

extern Ntop *ntop;

#endif /* _NTOP_CLASS_H_ */
