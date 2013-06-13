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
 * This program is distributed in the ho2pe that it will be useful,
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

/* ************************************ */

GenericHash::GenericHash(u_int _num_hashes, u_int _max_hash_size) {
  num_hashes = _num_hashes, max_hash_size = _max_hash_size, current_size = 0;

  table = new GenericHashEntry*[num_hashes];
  for(u_int i = 0; i < num_hashes; i++)
    table[i] = NULL;

  locks = new Mutex*[num_hashes];
  for(u_int i = 0; i < num_hashes; i++) locks[i] = new Mutex();

  last_purged_hash = _num_hashes - 1;
}

/* ************************************ */

GenericHash::~GenericHash() {
  for(u_int i = 0; i < num_hashes; i++)
    if(table[i] != NULL) {
      GenericHashEntry *head = table[i];

      while(head) {
	GenericHashEntry *next = head->next();

	delete(head);
	head = next;
      }
    }

  delete[] table;

  for(u_int i = 0; i < num_hashes; i++) delete(locks[i]);
  delete[] locks;
}

/* ************************************ */

bool GenericHash::add(GenericHashEntry *h) {
  if(current_size < max_hash_size) {
    u_int32_t hash = (h->key() % num_hashes);

    locks[hash]->lock(__FILE__, __LINE__);
    h->set_next(table[hash]);
    table[hash] = h, current_size++;
    locks[hash]->unlock(__FILE__, __LINE__);

    return(true);
  } else
    return(false);
}

/* ************************************ */

bool GenericHash::remove(GenericHashEntry *h) {
  u_int32_t hash = (h->key() % num_hashes);

  if(table[hash] == NULL)
    return(false);
  else {
    GenericHashEntry *head, *prev = NULL;
    bool ret;
    
    locks[hash]->lock(__FILE__, __LINE__);

    head = table[hash];
    while(head && (!head->equal(h))) {
      prev = head;
      head = head->next();
    }

    if(head) {
      if(prev != NULL)
	prev->set_next(head->next());
      else
	table[hash] = head->next();

      current_size--;

      ret = true;
    } else
      ret = false;
    
    locks[hash]->unlock(__FILE__, __LINE__);
    return(ret);
  }
}

/* ************************************ */

void GenericHash::walk(void (*walker)(GenericHashEntry *h, void *user_data), void *user_data) {
  if(ntop->getGlobals()->isShutdown()) return;

  for(u_int hash_id = 0; hash_id < num_hashes; hash_id++) {
    if(table[hash_id] != NULL) {
      GenericHashEntry *head;
      
      //ntop->getTrace()->traceEvent(TRACE_NORMAL, "[walk] Locking %d [%p]", hash_id, locks[hash_id]);
      locks[hash_id]->lock(__FILE__, __LINE__);
      head = table[hash_id];

      while(head) {
	GenericHashEntry *next = head->next();

	walker(head, user_data);
	head = next;
      } /* while */

      locks[hash_id]->unlock(__FILE__, __LINE__);
      // ntop->getTrace()->traceEvent(TRACE_NORMAL, "[walk] Unlocked %d", hash_id);
    }
  }
}

/* ************************************ */

u_int GenericHash::purgeIdle() {
  u_int i, num_purged = 0;

  if(ntop->getGlobals()->isShutdown()) return(0);

  for(u_int j = 0; j < num_hashes / PURGE_FRACTION; j++) {
  
    if (++last_purged_hash == num_hashes) last_purged_hash = 0;
    i = last_purged_hash;

    if(table[i] != NULL) {
      GenericHashEntry *head, *prev = NULL;

      // ntop->getTrace()->traceEvent(TRACE_NORMAL, "[purge] Locking %d", i);
      locks[i]->lock(__FILE__, __LINE__);
      head = table[i];

      while(head) {
	GenericHashEntry *next = head->next();

	if(head->idle()) {
	  if(prev == NULL) {
	    table[i] = next;	    
	  } else {
	    prev->set_next(next);
	  }

	  num_purged++, current_size--;
	  delete(head);
	  head = next;
	} else {
	  prev = head;
	  head = next;
	}
      } /* while */

      locks[i]->unlock(__FILE__, __LINE__);
      // ntop->getTrace()->traceEvent(TRACE_NORMAL, "[purge] Unlocked %d", i);
    }
  }

  return(num_purged);
}

/* ************************************ */

GenericHashEntry* GenericHash::findByKey(u_int32_t key) {
  u_int32_t hash = key % num_hashes;
  GenericHashEntry *head = table[hash];

  if(head == NULL) return(NULL);

  locks[hash]->lock(__FILE__, __LINE__);
  while(head != NULL) {
    if(head->key() == key)
      break;
    else      
      head = head->next();
  }
  
  locks[hash]->unlock(__FILE__, __LINE__);
  
  return(head);
}
