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

#include<string.h>
#include"ovndb.h"
#include<stdlib.h>
#include<stdio.h>

#define PR_INSERT_NODE_DATA  ovndb->prepared[0]	//TODO
#define PR_RETRIEVE_NODE_SUMMARY  ovndb->prepared[1]
#define PR_RETRIEVE_NODE_CONTENT  ovndb->prepared[2]

#define PR_INSERT_LINK  ovndb->prepared[3]
#define PR_RETRIEVE_LINK_SUMMARY  ovndb->prepared[4]
#define PR_RETRIEVE_LINK_CONTENT  ovndb->prepared[5]

#define PR_INSERT_NODE  ovndb->prepared[6]	//the node is a new one, not an updated one
#define PR_RETRIEVE_NODE  ovndb->prepared[7]

void print_error(CassFuture * future)
{
	CassString message = cass_future_error_message(future);
	fprintf(stderr, "Error: %.*s\n", (int)message.length, message.data);
}

CassCluster *create_cluster(const char **contact_points, int numb)
{
	CassCluster *cluster = cass_cluster_new();
	int i = 0;
	while (i < numb) {
		cass_cluster_setopt(cluster, CASS_OPTION_CONTACT_POINTS,
				    contact_points[i],
				    strlen(contact_points[i]));
		i++;
	}
	return cluster;
}

CassError connect_session(CassCluster * cluster, CassSession ** output)
{
	CassError rc = 0;
	CassFuture *future = cass_cluster_connect(cluster);

	*output = NULL;

	cass_future_wait(future);
	rc = cass_future_error_code(future);
	if (rc != CASS_OK) {
		print_error(future);
	} else {
		*output = cass_future_get_session(future);
	}
	cass_future_free(future);

	return rc;
}

void ovndb_init(ovndb_t ** ovndb_, const char *contact_points[], int numb,
		void *consensus_req_socket)
{
	ovndb_t *ovndb = *ovndb_;
	ovndb = calloc(1, sizeof(ovndb_t));
	ovndb->consensus_req_socket = consensus_req_socket;

	CassError rc = 0;
	ovndb->cluster = create_cluster(contact_points, numb);

	rc = connect_session(ovndb->cluster, &(ovndb->session));
	if (rc != CASS_OK) {
		exit(-1);
	}
//initialize prepared statements

	CassString query =
	    cass_string_init
	    ("INSERT INTO ordered_id.node (ordered_id,local_id,hist_id,last_local_id,node_summary,node_content,parent_id,parent_hist_id ) VALUES (?,?,?,?,?,?,?,? ) ");
	CassFuture *future = cass_session_prepare(ovndb->session, query);
	cass_future_wait(future);

	rc = cass_future_error_code(future);
	if (rc != CASS_OK) {
		print_error(future);
		exit(-1);
	} else {
		ovndb->prepared[0] = cass_future_get_prepared(future);
	}

	cass_future_free(future);

	query =
	    cass_string_init
	    ("SELECT node_summary,last_local_id FROM ordered_id.node WHERE ordered_id = ? and local_id = ? ");
	future = cass_session_prepare(ovndb->session, query);
	cass_future_wait(future);

	rc = cass_future_error_code(future);
	if (rc != CASS_OK) {
		print_error(future);
		exit(-1);
	} else {
		ovndb->prepared[1] = cass_future_get_prepared(future);
	}

	cass_future_free(future);

	query =
	    cass_string_init
	    ("SELECT node_content,last_local_id FROM ordered_id.node WHERE ordered_id = ? and local_id IN (?,?) ");
	future = cass_session_prepare(ovndb->session, query);
	cass_future_wait(future);

	rc = cass_future_error_code(future);
	if (rc != CASS_OK) {
		print_error(future);
		exit(-1);
	} else {
		ovndb->prepared[2] = cass_future_get_prepared(future);
	}

	cass_future_free(future);

	query =
	    cass_string_init
	    ("INSERT INTO ordered_id.link (ordered_id,local_id,hist_id,last_local_id,link_summary,link_content,parent_id,parent_hist_id ) VALUES (?,?,?i,?,?,?,?,? ) ");
	future = cass_session_prepare(ovndb->session, query);
	cass_future_wait(future);

	rc = cass_future_error_code(future);
	if (rc != CASS_OK) {
		print_error(future);
		exit(-1);
	} else {
		ovndb->prepared[3] = cass_future_get_prepared(future);
	}

	cass_future_free(future);

	query =
	    cass_string_init
	    ("SELECT link_summary,last_local_id FROM ordered_id.link WHERE ordered_id = ? and local_id = ? ");
	future = cass_session_prepare(ovndb->session, query);
	cass_future_wait(future);

	rc = cass_future_error_code(future);
	if (rc != CASS_OK) {
		print_error(future);
		exit(-1);
	} else {
		ovndb->prepared[4] = cass_future_get_prepared(future);
	}

	cass_future_free(future);

	query =
	    cass_string_init
	    ("SELECT link_content,last_local_id FROM ordered_id.link_content WHERE ordered_id = ? and local_id IN (?,?) ");
	future = cass_session_prepare(ovndb->session, query);
	cass_future_wait(future);

	rc = cass_future_error_code(future);
	if (rc != CASS_OK) {
		print_error(future);
		exit(-1);
	} else {
		ovndb->prepared[5] = cass_future_get_prepared(future);
	}

	cass_future_free(future);

//node is null at initialization
	query =
	    cass_string_init
	    ("INSERT INTO ordered_id.graph (ordered_id,set_id,local_id,local_set_id,hist_id,last_local_id,last_set_id) VALUES (?,?,?,?,?,?,?) ");
	future = cass_session_prepare(ovndb->session, query);
	cass_future_wait(future);

	rc = cass_future_error_code(future);
	if (rc != CASS_OK) {
		print_error(future);
		exit(-1);
	} else {
		ovndb->prepared[6] = cass_future_get_prepared(future);
	}

	cass_future_free(future);

	query =
	    cass_string_init
	    ("SELECT node,last_local_id FROM ordered_id.graph WHERE ordered_id = ? and local_id = ? ");
	future = cass_session_prepare(ovndb->session, query);
	cass_future_wait(future);

	rc = cass_future_error_code(future);
	if (rc != CASS_OK) {
		print_error(future);
		exit(-1);
	} else {
		ovndb->prepared[7] = cass_future_get_prepared(future);
	}

	cass_future_free(future);

}

void ovndb_close(ovndb_t * ovndb)
{
	CassFuture *close_future = NULL;
	close_future = cass_session_close(ovndb->session);
	cass_future_wait(close_future);
	cass_cluster_free(ovndb->cluster);

}

/*
int ovndb_new_node_data(ovndb_t * ovndb, int64_t id, json_t * new_node_data)
{

	char *errptr = NULL;
	json_t *old_node = ovndb_retrieve_node(ovndb, id);
	if (!old_node) {
		return 0;
	}
	json_t *summary = json_object_get(new_node_data,
					  "summary");
	json_t *content = json_object_get(new_node_data,
					  "content");

	leveldb_writebatch_t *wb = leveldb_writebatch_create();
	if (summary != NULL) {
		json_object_set(json_object_get(old_node, "nodeData"),
				"summary", summary);
		const char *str_node = json_dumps(old_node, JSON_COMPACT);
		leveldb_writebatch_put(wb, (const char *)&id, sizeof(int64_t),
				       str_node, strlen(str_node));
		free((char *)str_node);
	}
	char key[9];
	if (content != NULL) {
		const char *str = json_dumps(content, JSON_COMPACT);
		memcpy(key, &id, sizeof(int64_t));
		key[9] = CONTENT;
		leveldb_writebatch_put(wb, (const char *)key, 9,
				       str, strlen(str));

		free((char *)str);
	}
	leveldb_write(ovndb->db, ovndb->writeoptions, wb, &errptr);
	leveldb_writebatch_destroy(wb);

	if (errptr) {
		printf("\n%s", errptr);
		exit(1);
	}

	return 1;

}
*/

#define OVNDB_NEW_NODE_GOT_CONSENSUS 1
#define OVNDB_NEW_NODE_SENT_REQUEST 2

int64_t ovndb_insert_node(ovndb_t * ovndb, db_new_node_t * request,
			  int32_t * set_id, char *hid)
{

	switch (request->state) {

	case 0:;

		int32_t type = CONSENSUS_TYPE_GRAPH_NODE;
		zmsg_t *msg = zmsg_new();

		zmsg_addmem(msg, &type, sizeof(int32_t));
		zmsg_addmem(msg, NULL, 0);

		zmsg_send(&msg, ovndb->consensus_req_socket);
		request->state = OVNDB_NEW_NODE_GOT_CONSENSUS;
		break;

	case OVNDB_NEW_NODE_GOT_CONSENSUS:;

		msg = request->consensus_msg;
		zmsg_first(msg);
		zframe_t *frame = zmsg_next(msg);
		int64_t id;
		memcpy(&id, zframe_data(frame), sizeof(int64_t));
		frame = zmsg_next(msg);
		memcpy(set_id, zframe_data(frame), sizeof(int32_t));
		frame = zmsg_next(msg);
		memcpy(hid, zframe_data(frame), SHA512_LENGTH);

		CassStatement *statement;
		CassFuture *future;

		statement =
		    cass_prepared_bind(PR_INSERT_NODE, 6, CASS_CONSISTENCY_ONE);

		cass_statement_bind_int64(statement, 0, id);
		cass_statement_bind_int64(statement, 1, *set_id);
		cass_statement_bind_int64(statement, 3, id);
		cass_statement_bind_int64(statement, 4, *set_id);
		cass_statement_bind_string(statement, 5,
					   cass_string_init2(hid,
							     SHA512_LENGTH));
		cass_statement_bind_int64(statement, 6, id);
		cass_statement_bind_int64(statement, 7, *set_id);

		future = cass_session_execute(ovndb->session, statement);

		request->future = malloc(sizeof(CassFuture *));
		request->future[request->conq] = future;
		request->conq++;
		request->state = OVNDB_NEW_NODE_SENT_REQUEST;
		return id;

		break;

	case OVNDB_NEW_NODE_SENT_REQUEST:;
		cass_future_free(request->future[request->conq - 1]);
		request->conq--;
		free(request->future);
		break;

	}
//To remove warnings
	return 0;
}

/*

int ovndb_delete_link(ovndb_t * ovndb, json_t * link)
{

	char *errptr = NULL;
	int64_t origId = json_integer_value(json_object_get(link, "origId"));
	int64_t endId = json_integer_value(json_object_get(link, "endId"));
	int64_t id =
	    json_integer_value(json_object_get
			       (json_object_get(link, "linkData"), "id"));

	json_t *origNode = ovndb_retrieve_node(ovndb, origId);
	json_t *endNode = ovndb_retrieve_node(ovndb, endId);

	if (!(origNode && endNode))
		return 0;

	json_t *origOut = json_object_get(origNode, "output");
	json_t *endIn = json_object_get(endNode, "input");

	size_t index;
	json_t *value;
	int found_it = 0;
	json_array_foreach(origOut, index, value) {
		int64_t id2 =
		    json_integer_value(json_object_get
				       (json_object_get(value, "linkData"),
					"id"));
		int64_t nid =
		    json_integer_value(json_object_get(value, "endId"));
		if ((id2 == id) && (endId == nid)) {
			json_array_remove(origOut, index);
			found_it++;
		}
	}
	json_array_foreach(endIn, index, value) {
		int64_t id2 =
		    json_integer_value(json_object_get
				       (json_object_get(value, "linkData"),
					"id"));
		int64_t nid =
		    json_integer_value(json_object_get(value, "origId"));
		if ((id2 == id) && (origId == nid)) {
			json_array_remove(endIn, index);
			found_it++;
		}
	}
	if (found_it != 2)
		return 0;

	const char *str_origNode = json_dumps(origNode, JSON_COMPACT);
	const char *str_endNode = json_dumps(endNode, JSON_COMPACT);

	leveldb_writebatch_t *wb = leveldb_writebatch_create();
	leveldb_writebatch_put(wb, (const char *)&(origId),
			       sizeof(int64_t), str_origNode,
			       strlen(str_origNode));
	leveldb_writebatch_put(wb, (const char *)&(endId), sizeof(int64_t),
			       str_endNode, strlen(str_endNode));

	leveldb_write(ovndb->db, ovndb->writeoptions, wb, &errptr);
	leveldb_writebatch_destroy(wb);
	free((char *)str_origNode);
	free((char *)str_endNode);

	json_decref(origNode);
	json_decref(endNode);

	if (errptr) {
		printf("\n%s", errptr);
		exit(1);
	}

	return 1;
}

int64_t ovndb_save_link(ovndb_t * ovndb, json_t * link)
{

	char *errptr = NULL;

//new linkData or existing data

	int64_t origId = json_integer_value(json_object_get(link, "origId"));
	json_t *origNode = ovndb_retrieve_node(ovndb, origId);
	int found_it = 0;

	int64_t tid =
	    json_integer_value(json_object_get
			       (json_object_get(link, "linkData"), "id"));
	int64_t id;

	if (tid != -1) {
//the same data must be at the same node
		json_t *origOut = json_object_get(origNode, "output");

		size_t index;
		json_t *value;
		json_array_foreach(origOut, index, value) {
			int64_t id2 =
			    json_integer_value(json_object_get
					       (json_object_get
						(value, "linkData"),
						"id"));
			if (id2 == tid) {
				found_it = 1;
				id = tid;
			}
		}

		if (!found_it) {
			return 0;
		}

	} else {
		id = ovndb->nextId;
		json_object_set_new(json_object_get(link, "linkData"), "id",
				    json_integer(id));
	}

	int64_t endId = json_integer_value(json_object_get(link, "endId"));
	if (origId == endId)
		return 0;

	json_t *endNode = ovndb_retrieve_node(ovndb, endId);

	if (!(origNode && endNode))
		return 0;

	json_array_append(json_object_get(origNode, "output"), link);
	json_array_append(json_object_get(endNode, "input"), link);

	const char *str_origNode = json_dumps(origNode, JSON_COMPACT);
	const char *str_endNode = json_dumps(endNode, JSON_COMPACT);

	//update the last id 
	leveldb_writebatch_t *wb = leveldb_writebatch_create();
	int64_t zero = 0;
	leveldb_writebatch_put(wb, (const char *)&zero, sizeof(int64_t),
			       (const char *)&(ovndb->nextId), sizeof(int64_t));

	leveldb_writebatch_put(wb, (const char *)&(origId),
			       sizeof(int64_t), str_origNode,
			       strlen(str_origNode));
	leveldb_writebatch_put(wb, (const char *)&(endId), sizeof(int64_t),
			       str_endNode, strlen(str_endNode));

	leveldb_write(ovndb->db, ovndb->writeoptions, wb, &errptr);
	leveldb_writebatch_destroy(wb);
	ovndb->nextId++;
	free((char *)str_origNode);
	free((char *)str_endNode);

	json_decref(origNode);
	json_decref(endNode);

	if (errptr) {
		printf("\n%s", errptr);
		exit(1);
	}
	return id;

}

int ovndb_delete_node(ovndb_t * ovndb, int64_t id)
{
	json_t *node = ovndb_retrieve_node(ovndb, id);
	if (json_array_size(json_object_get(node, "input"))
	    || json_array_size(json_object_get(node, "output"))) {
		json_decref(node);
		return 0;
	} else {
		char *errptr = NULL;
		leveldb_delete(ovndb->db, ovndb->writeoptions,
			       (const char *)&id, sizeof(int64_t), &errptr);

		if (errptr) {
			printf("\n%s", errptr);
			exit(1);
		}
		json_decref(node);
		return 1;
	}
}

*/

#define OVNDB_RETRIEVE_NODE_GOT_IT 1

json_t *ovndb_retrieve_node(ovndb_t * ovndb, db_retrieve_node_t * request)
{
	switch (request->state) {

	case 0:;

		CassStatement *statement;
		CassFuture *future;

		statement =
		    cass_prepared_bind(PR_RETRIEVE_NODE_SUMMARY, 2,
				       CASS_CONSISTENCY_ONE);

		cass_statement_bind_int64(statement, 0, request->ancestorId);
		cass_statement_bind_int64(statement, 1, request->id);
		future = cass_session_execute(ovndb->session, statement);

		request->future = malloc(sizeof(CassFuture *));
		request->future[request->conq] = future;
		request->conq++;
		request->state = OVNDB_RETRIEVE_NODE_GOT_IT;
		break;

	case OVNDB_RETRIEVE_NODE_GOT_IT:;
		CassError rc = 0;
		future = request->future[request->conq - 1];

		rc = cass_future_error_code(future);
		if (rc != CASS_OK) {
			print_error(future);
			exit(-1);
		} else {
			const CassResult *result =
			    cass_future_get_result(future);
			const CassRow *row = cass_result_first_row(result);

			CassString node;
			cass_value_get_string(cass_row_get_column(row, 1),
					      &node);

			json_error_t jerror;
			json_t *node_json =
			    json_loadb(node.data, node.length, 0, &jerror);
			if (!node_json) {
				printf("ovndb_retrieve json error:%s",
				       jerror.text);
			}

			cass_result_free(result);
			cass_future_free(future);
			request->conq--;
			free(request->future);
			return node_json;
		}
		break;
	}

//To remove warnings
	return NULL;
}
