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

#include<jansson.h>
#include<cassandra.h>
#include<czmq.h>
#include"lib_sha512/sha512.h"
#include"consensus.pb-c.h"

#define NUM_CONCURRENT_REQUESTS 4096
#define SLEEP_MS 5

//multiple futures per request can happen concurrently
//this is an interface, others structs need to cast into it.
struct db_request_t {
	int state;
	int conq;
	CassFuture **future;

};

typedef struct db_request_t db_request_t;

struct db_retrieve_node_t {
	int state;
	int conq;
	CassFuture **future;
	int64_t ancestorId;
	int64_t id;
};

typedef struct db_retrieve_node_t db_retrieve_node_t;

struct db_new_node_t {
	int state;
	int conq;
	CassFuture **future;
	Consensus con;
                int64_t ordered_id;
        int32_t set_id;
        char hid[SHA512_LENGTH];
};

typedef struct db_new_node_t db_new_node_t;

struct ovndb_t {
	CassCluster *cluster;
	CassSession *session;
	const CassPrepared *prepared[8];
	void *consensus_req_socket;
};

typedef struct ovndb_t ovndb_t;

void ovndb_init(ovndb_t ** ovndb, const char *contact_points,
		void *consensus_req_socket);

void ovndb_close(ovndb_t * ovndb);

int64_t ovndb_insert_node(ovndb_t * ovndb, db_new_node_t * request,
			  int32_t * set_id, char *hid);
int64_t ovndb_save_link(ovndb_t * ovndb, json_t * link);

json_t *ovndb_retrieve_node(ovndb_t * ovndb, db_retrieve_node_t * request);

//returns 1 if the node was deleted
int ovndb_delete_node(ovndb_t * ovndb, int64_t id);

int ovndb_delete_link(ovndb_t * ovndb, json_t * link);

int64_t ovndb_save_link(ovndb_t * ovndb, json_t * link);

int ovndb_new_node_data(ovndb_t * ovndb, int64_t id, json_t * new_node_data);
#endif
