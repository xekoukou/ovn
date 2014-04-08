/*
    Copyright contributors as noted in the AUTHORS file.
                
    This file is part of PLATANOS.

    PLATANOS is free software; you can redistribute it and/or modify it under
    the terms of the GNU Affero General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.
            
    PLATANOS is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.
        
    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include<leveldb/c.h>
#include<string.h>
#include"ovndb.h"
#include<stdlib.h>
#include<stdio.h>

void
ovndb_init (ovndb_t ** ovndb)
{
  *ovndb = malloc (sizeof (ovndb_t));

  char *errptr = NULL;

  leveldb_options_t *options = leveldb_options_create ();
  leveldb_options_set_create_if_missing (options, 1);

  (*ovndb)->options = options;

  leveldb_readoptions_t *readoptions = leveldb_readoptions_create ();
  (*ovndb)->readoptions = readoptions;
  leveldb_writeoptions_t *writeoptions = leveldb_writeoptions_create ();
  (*ovndb)->writeoptions = writeoptions;

  leveldb_writeoptions_set_sync (writeoptions, 1);

  (*ovndb)->db = leveldb_open (options, "./ovndb", &errptr);
  if (errptr)
    {
      printf ("\n%s", errptr);
      exit (1);
    }
//obtain the nextId
  int64_t zero = 0;
  size_t vallen = sizeof (int64_t);
  (*ovndb)->nextId =
    *(leveldb_get ((*ovndb)->db, readoptions, (char *) &zero, sizeof (int64_t),
		   &vallen, &errptr));
  (*ovndb)->nextId++;
}

//needs a few seconds to close
//put a sleep after it
void
ovndb_close (ovndb_t * ovndb)
{
  leveldb_close (ovndb->db);

}

void
ovndb_insert_node (ovndb_t * ovndb, json_t * node)
{

  char *errptr = NULL;
  int64_t id = json_integer_value(json_object_get (node, "id"));

  if (id != -1)
    {
      const char *str_node = json_dumps (node, JSON_COMPACT);
      leveldb_put(ovndb->db,
       ovndb->writeoptions, (const char *) &id,
       sizeof (int64_t), (const char *)
       str_node, 2 * sizeof (int64_t), &errptr);
    }
  else
    {
      id = ovndb->nextId;
      json_object_set_new (node, "id", json_integer (id));
      const char *str_node = json_dumps (node, JSON_COMPACT);

      //update the last id 
      leveldb_writebatch_t *wb = leveldb_writebatch_create ();
      int64_t zero = 0;
      leveldb_writebatch_put (wb, (const char *)&zero, sizeof (int64_t),
			     (const char *)&(ovndb->nextId), sizeof (int64_t));

      leveldb_writebatch_put (wb,(const char *) &(ovndb->nextId), sizeof (int64_t),
			     str_node, sizeof (int64_t));

      leveldb_write (ovndb->db, ovndb->writeoptions, wb, &errptr);
      //TODO catch the error
      leveldb_writebatch_destroy (wb);
      ovndb->nextId++;

    }

  if (errptr)
    {
      printf ("\n%s", errptr);
      exit (1);
    }

}

void
ovndb_delete_node (ovndb_t * ovndb, int64_t id)
{

  char *errptr = NULL;
  leveldb_delete (ovndb->db, ovndb->writeoptions, (const char *)&id, sizeof (int64_t),
		  &errptr);

  if (errptr)
    {
      printf ("\n%s", errptr);
      exit (1);
    }

}

//the returned value must be copied, it is temporary
char *
ovndb_retrieve_node (ovndb_t * ovndb, int64_t id, int64_t * length)
{
  char *errptr;
  size_t vallen = sizeof (int64_t);

  char *tempval =
    leveldb_get (ovndb->db, ovndb->readoptions, (const char *)&id, sizeof (int64_t),
		 &vallen, &errptr);
  *length = vallen;
  return tempval;

}
