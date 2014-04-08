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

#ifndef OVNDB_H
#define OVNDB_H

//leveldb
#include<leveldb/c.h>
#include<jansson.h>

typedef struct
{
  int64_t nextId;
  leveldb_t *db;
  leveldb_options_t *options;
  leveldb_readoptions_t *readoptions;
  leveldb_writeoptions_t *writeoptions;

} ovndb_t;

void ovndb_init (ovndb_t ** ovndb);

//sleep a few seconds after
void ovndb_close (ovndb_t * ovndb);

// id of -1 results in obtaining a new id
void ovndb_insert_node (ovndb_t * ovndb, json_t * node);

char *ovndb_retrieve_node (ovndb_t * ovndb, int64_t id, int64_t * length);

void ovndb_delete_node (ovndb_t * ovndb, int64_t id);
#endif