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

#ifndef _NTOP_WIN32_H_
#define _NTOP_WIN32_H_


#include <winsock2.h> /* winsock.h is included automatically */
#include <ws2tcpip.h>
#include <process.h>
#include <io.h>

extern "C" {
#include <getopt.h> /* getopt from: http://www.pwilson.net/sample.html. */
}

#include <process.h> /* for getpid() and the exec..() family */
#include <direct.h>  /* mkdir */



/* Values for the second argument to access. These may be OR'd together.  */
#define R_OK    4       /* Test for read permission.  */
#define W_OK    2       /* Test for write permission.  */
//#define   X_OK    1       /* execute permission - unsupported in windows*/
#define F_OK    0       /* Test for existence.  */

#define access _access
#define ftruncate _chsize

#if 0
#define pthread_t              DWORD
#define pthread_mutex_t        HANDLE
#define pthread_rwlock_t       HANDLE
#define pthread_rwlock_init                     pthread_mutex_init
#define pthread_rwlock_wrlock                   pthread_mutex_lock
#define pthread_rwlock_unlock                   pthread_mutex_unlock
#endif

typedef HANDLE pthread_mutex_t;
typedef struct {HANDLE signal, broadcast;} pthread_cond_t;
typedef HANDLE pthread_t;

extern int pthread_create(pthread_t *threadId, void* notUsed, void *(*__start_routine) (void *), void* userParm);
extern void pthread_detach(pthread_t *threadId);
extern int pthread_join(pthread_t thread, void **value_ptr);

extern int pthread_mutex_init(pthread_mutex_t *mutex, void* notused);
extern int pthread_mutex_destroy(pthread_mutex_t *mutex);
extern int pthread_mutex_lock(pthread_mutex_t *mutex);
extern int pthread_mutex_trylock(pthread_mutex_t *mutex);
extern int pthread_mutex_unlock(pthread_mutex_t *mutex);

extern char *strtok_r(char *s, const char *delim, char **save_ptr);

#define strdup(a) _strdup(a)

/* mongoose.c */
#define HAS_POLL
#define NO_CGI

#ifndef PATH_MAX
#define PATH_MAX MAX_PATH
#endif

struct dirent {
  char d_name[PATH_MAX];
};

typedef struct DIR {
  HANDLE   handle;
  WIN32_FIND_DATAW info;
  struct dirent  result;
} DIR;

extern struct dirent *readdir(DIR *dir);
extern int closedir(DIR *dir);
extern DIR * opendir(const char *name);


/* getopt.h */
#define __GNU_LIBRARY__ 1

#ifndef __GNUC__
typedef unsigned char  u_char;
typedef unsigned short u_short;
typedef unsigned int   uint;
typedef unsigned long  u_long;
#endif

typedef u_char  u_int8_t;
typedef u_short u_int16_t;
typedef uint   u_int32_t;
typedef unsigned __int64 u_int64_t;

#define _WS2TCPIP_H_ /* Avoid compilation problems */
#define HAVE_SIN6_LEN

/* IPv6 address */
/* Already defined in WS2tcpip.h */
struct win_in6_addr
{
  union
  {
    u_int8_t u6_addr8[16];
    u_int16_t u6_addr16[8];
    u_int32_t u6_addr32[4];
  } in6_u;
#ifdef s6_addr
#undef s6_addr
#endif

#ifdef s6_addr16
#undef s6_addr16
#endif

#ifdef s6_addr32
#undef s6_addr32
#endif

#define s6_addr                 in6_u.u6_addr8
#define s6_addr16               in6_u.u6_addr16
#define s6_addr32               in6_u.u6_addr32

};

#define in6_addr win_in6_addr

struct ip6_hdr
{
  union
  {
    struct ip6_hdrctl
    {
      u_int32_t ip6_un1_flow;   /* 4 bits version, 8 bits TC,
				   20 bits flow-ID */
      u_int16_t ip6_un1_plen;   /* payload length */
      u_int8_t  ip6_un1_nxt;    /* next header */
      u_int8_t  ip6_un1_hlim;   /* hop limit */
    } ip6_un1;
    u_int8_t ip6_un2_vfc;       /* 4 bits version, top 4 bits tclass */
  } ip6_ctlun;
  struct in6_addr ip6_src;      /* source address */
  struct in6_addr ip6_dst;      /* destination address */
};

/* Generic extension header.  */
struct ip6_ext
{
  u_int8_t  ip6e_nxt;		/* next header.  */
  u_int8_t  ip6e_len;		/* length in units of 8 octets.  */
};

#ifndef S_ISDIR
#define S_ISDIR(mode)  (((mode) & S_IFMT) == S_IFDIR)
#endif

#ifndef S_ISREG
#define S_ISREG(mode)  (((mode) & S_IFMT) == S_IFREG)
#endif

#define localtime_r(a, b) localtime(a)

extern unsigned int sleep(unsigned int seconds);

extern int inet_aton(const char *cp, struct in_addr *addr);
//extern int inet_pton(int af, const char *src, void *dst);


#endif /* _NTOP_WIN32_H_ */
