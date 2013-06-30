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

#ifdef WIN32
#include <shlobj.h> /* SHGetFolderPath() */
#endif

Ntop *ntop;

/* ******************************************* */

Ntop::Ntop(char *appName) {
  globals = new NtopGlobals();
  pa = new PeriodicActivities();
  address = new AddressResolution();
  categorization = NULL;
  custom_ndpi_protos = NULL;
  rrd_lock = new Mutex(); /* FIX: one day we need to use the reentrant RRD API */
  prefs = NULL;
  
#ifndef WIN32
  getcwd(startup_dir, sizeof(startup_dir));
#endif

#ifdef WIN32
  if(SHGetFolderPath(NULL, CSIDL_PERSONAL, NULL, SHGFP_TYPE_CURRENT, working_dir) != S_OK) {
    strcpy(working_dir, "C:\\Windows\\Temp"); // Fallback: it should never happen
  }

  // Get the full path and filename of this program
  if(GetModuleFileName(NULL, startup_dir, sizeof(startup_dir)) == 0) {
	  startup_dir[0] = '\0';
  } else {
	  for(int i=strlen(startup_dir)-1; i>0; i--)
		  if(startup_dir[i] == '\\') {
			  startup_dir[i] = '\0';
			  break;
		  }
  }
  strcpy(install_dir, startup_dir);
#else
  strcpy(working_dir, CONST_DEFAULT_WRITABLE_DIR);
  if(getcwd(install_dir, sizeof(install_dir)) == NULL) strcpy(install_dir, ".");
  getcwd(startup_dir, sizeof(startup_dir));
#endif

  // printf("--> %s [%s]\n", startup_dir, appName);
}

/* ******************************************* */

void Ntop::registerPrefs(Prefs *_prefs, Redis *_redis) {
  struct stat statbuf;

  prefs = _prefs, redis = _redis;

  if(stat(prefs->get_data_dir(), &statbuf)
     || (!(statbuf.st_mode & S_IFDIR)) /* It's not a directory */
     || (!(statbuf.st_mode & S_IWRITE)) /* It's not writable    */) {
    ntop->getTrace()->traceEvent(TRACE_ERROR, "Invalid directory %s specified", prefs->get_data_dir());
    exit(-1);
  }

  if(stat(prefs->get_callbacks_dir(), &statbuf)
     || (!(statbuf.st_mode & S_IFDIR)) /* It's not a directory */
     || (!(statbuf.st_mode & S_IWRITE)) /* It's not writable    */) {
    ntop->getTrace()->traceEvent(TRACE_ERROR, "Invalid directory %s specified", prefs->get_callbacks_dir());
    exit(-1);
  }

  setLocalNetworks(prefs->get_local_networks());
}

/* ******************************************* */

Ntop::~Ntop() {
  if(iface) delete iface;
  if(httpd) delete httpd;
  if(custom_ndpi_protos) delete(custom_ndpi_protos);

  delete rrd_lock;
  delete address;
  delete pa;
  delete geo;
  delete redis;
  delete globals;
}

/* ******************************************* */

void Ntop::start() {
  getTrace()->traceEvent(TRACE_NORMAL, "Welcome to ntopng %s v.%s (%s) - (C) 1998-13 ntop.org",
			 PACKAGE_MACHINE, PACKAGE_VERSION, NTOP_SVN_REVISION);

  pa->startPeriodicActivitiesLoop();
  address->startResolveAddressLoop();
  if(categorization) categorization->startCategorizeCategorizationLoop();
}

/* ******************************************* */

void Ntop::loadGeolocation(char *dir) {
  geo = new Geolocation(dir);
}

/* ******************************************* */

void Ntop::setWorkingDir(char *dir) {
  snprintf(working_dir, sizeof(working_dir), "%s", dir);
  removeTrailingSlash(working_dir);
};

/* ******************************************* */

void Ntop::removeTrailingSlash(char *str) {
  int len = strlen(str)-1;

  if((len > 0) 
     && ((str[len] == '/') || (str[len] == '\\')))
    str[len] = '\0';
}

/* ******************************************* */

void Ntop::setCustomnDPIProtos(char *path) {
  if(path != NULL) {
    if(custom_ndpi_protos != NULL) free(custom_ndpi_protos);
    custom_ndpi_protos = strdup(path);
  }
}

/* ******************************************* */

void Ntop::getUsers(lua_State* vm) {
  char **usernames;
  char *username, *holder;
  char key[64], val[64];
  int rc, i;

  lua_newtable(vm);

  if((rc = ntop->getRedis()->keys("user.*.password", &usernames)) <= 0) {
    return;
  }

  for (i = 0; i < rc; i++) {
    if (usernames[i] == NULL) continue; /* safety check */
    if (strtok_r(usernames[i], ".", &holder) == NULL) continue;
    if ((username = strtok_r(NULL, ".", &holder)) == NULL) continue;

    lua_newtable(vm);

    snprintf(key, sizeof(key), "user.%s.full_name", username);
    if(ntop->getRedis()->get(key, val, sizeof(val)) >= 0)
      lua_push_str_table_entry(vm, "full_name", val);
    else
      lua_push_str_table_entry(vm, "full_name", (char*) "unknown");

    snprintf(key, sizeof(key), "user.%s.group", username);
    if(ntop->getRedis()->get(key, val, sizeof(val)) >= 0)
      lua_push_str_table_entry(vm, "group", val);
    else
      lua_push_str_table_entry(vm, "group", (char*)"unknown");

    lua_pushstring(vm, username);
    lua_insert(vm, -2);
    lua_settable(vm, -3);

    free(usernames[i]);
  }

  free(usernames);
}

/* ******************************************* */

// Return 1 if username/password is allowed, 0 otherwise.
int Ntop::checkUserPassword(const char *user, const char *password) {
  // In production environment we should ask an authentication system
  // to authenticate the user.
  char key[64], val[64];
  char password_hash[33];

  if(user == NULL) return(false);

  snprintf(key, sizeof(key), "user.%s.password", user);

  if(ntop->getRedis()->get(key, val, sizeof(val)) < 0) {
    return(false);
  } else {
    mg_md5(password_hash, password, NULL);
    return(strcmp(password_hash, val) == 0);
  }
}

/* ******************************************* */

int Ntop::resetUserPassword(char *username, char *old_password, char *new_password) {
  char key[64];
  char password_hash[33];

  if (!checkUserPassword(username, old_password))
    return(false);

  snprintf(key, sizeof(key), "user.%s.password", username);

  mg_md5(password_hash, new_password, NULL);

  if(ntop->getRedis()->set(key, password_hash, 0) < 0)
    return(false);

  return(true);
}

/* ******************************************* */

int Ntop::addUser(char *username, char *full_name, char *password) {
  char key[64];
  char password_hash[33];

  // FIX add a seed
  mg_md5(password_hash, password, NULL);

  snprintf(key, sizeof(key), "user.%s.full_name", username);
  ntop->getRedis()->set(key, full_name, 0);

  snprintf(key, sizeof(key), "user.%s.group", username);
  ntop->getRedis()->set(key, (char*) "administrator" /* TODO */, 0);

  snprintf(key, sizeof(key), "user.%s.password", username);
  return(ntop->getRedis()->set(key, password_hash, 0) >= 0);
}

/* ******************************************* */

int Ntop::deleteUser(char *username) {
  char key[64];

  snprintf(key, sizeof(key), "user.%s.full_name", username);
  ntop->getRedis()->del(key);

  snprintf(key, sizeof(key), "user.%s.group", username);
  ntop->getRedis()->del(key);

  snprintf(key, sizeof(key), "user.%s.password", username);
  return(ntop->getRedis()->del(key) >= 0);
}

/* ******************************************* */

void Ntop::fixPath(char *str) {
#ifdef WIN32
  for(int i=0; str[i] != '\0'; i++)
    if(str[i] == '/') str[i] = '\\';
#endif
}

/* ******************************************* */

char* Ntop::getValidPath(char *__path) {
  char _path[MAX_PATH];
  struct stat buf;
#ifdef WIN32
  const char *install_dir = (const char *)get_install_dir();
#endif
  const char* dirs[] = {
    startup_dir,
#ifndef WIN32
    "/usr/local/share/ntopng",
#else
    install_dir,
#endif
    NULL
  };

  if(strncmp(__path, "./", 2) == 0) {
    snprintf(_path, MAX_PATH, "%s/%s", startup_dir, &__path[2]);
    fixPath(_path);

    if(stat(_path, &buf) == 0)
      return(strdup(_path));
  }

  if((__path[0] == '/') || (__path[0] == '\\')) {
    /* Absolute paths */
    
    if(stat(_path, &buf) == 0) {
      return(strdup(_path));
    }
  } else
    snprintf(_path, MAX_PATH, "%s", __path);

  /* relative paths */
  for(int i=0; dirs[i] != NULL; i++) {
    char path[256];

    snprintf(path, sizeof(path), "%s/%s", dirs[i], _path);
    fixPath(path);

    if(stat(path, &buf) == 0) {
      return(strdup(path));
    }
  }

  return(NULL);
}

/* ******************************************* */

void Ntop::daemonize() {
#ifndef WIN32
  int childpid;

  signal(SIGHUP, SIG_IGN);
  signal(SIGCHLD, SIG_IGN);
  signal(SIGQUIT, SIG_IGN);

  if((childpid = fork()) < 0)
    ntop->getTrace()->traceEvent(TRACE_ERROR, "Occurred while daemonizing (errno=%d)", errno);
  else {
    if(!childpid) { /* child */
      int rc;
      
      //ntop->getTrace()->traceEvent(TRACE_NORMAL, "Bye bye: I'm becoming a daemon...");

#if 1
      rc = chdir("/");
      if(rc != 0) 
	ntop->getTrace()->traceEvent(TRACE_ERROR, "Error while moving to / directory");

      setsid();  /* detach from the terminal */

      fclose(stdin);
      fclose(stdout);
      /* fclose(stderr); */

      /*
       * clear any inherited file mode creation mask
       */
      umask(0);

      /*
       * Use line buffered stdout
       */
      /* setlinebuf (stdout); */
      setvbuf(stdout, (char *)NULL, _IOLBF, 0);
#endif
    } else { /* father */
      ntop->getTrace()->traceEvent(TRACE_NORMAL, "Parent process is exiting (this is normal)");
      exit(0);
    }
  }
#endif
}

/* ******************************************* */
 
void Ntop::setLocalNetworks(char *nets) {
  ntop->getTrace()->traceEvent(TRACE_NORMAL, "Setting local networks to %s", nets);
  address->setLocalNetworks(nets);            
};
