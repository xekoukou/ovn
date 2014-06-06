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

#define CONTENT 1
#define NET_LINK 2

void ovndb_init(ovndb_t ** ovndb)
{
	*ovndb = malloc(sizeof(ovndb_t));

	char *errptr = NULL;

	leveldb_options_t *options = leveldb_options_create();

	(*ovndb)->options = options;

	leveldb_readoptions_t *readoptions = leveldb_readoptions_create();
	(*ovndb)->readoptions = readoptions;
	leveldb_writeoptions_t *writeoptions = leveldb_writeoptions_create();
	(*ovndb)->writeoptions = writeoptions;

	leveldb_writeoptions_set_sync(writeoptions, 1);

	(*ovndb)->db = leveldb_open(options, "./ovndb", &errptr);
	if (errptr) {
		printf("\n%s", errptr);
		exit(1);
	}
//obtain the nextId
	int64_t zero = 0;
	size_t vallen = sizeof(int64_t);
	int64_t *value;
	value = (int64_t *) leveldb_get
	    ((*ovndb)->db, readoptions, (char *)&zero, sizeof(int64_t),
	     &vallen, &errptr);

	(*ovndb)->nextId = *value;
	(*ovndb)->nextId++;
}

//needs a few seconds to close
//put a sleep after it
void ovndb_close(ovndb_t * ovndb)
{
	leveldb_close(ovndb->db);

}

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

int64_t ovndb_insert_node(ovndb_t * ovndb, json_t * node)
{

	char *errptr = NULL;
	int64_t id = ovndb->nextId;
	json_object_set_new(node, "id", json_integer(id));
	const char *str_node = json_dumps(node, JSON_COMPACT);

	//update the last id 
	leveldb_writebatch_t *wb = leveldb_writebatch_create();
	int64_t zero = 0;
	leveldb_writebatch_put(wb, (const char *)&zero, sizeof(int64_t),
			       (const char *)&(ovndb->nextId), sizeof(int64_t));

	leveldb_writebatch_put(wb, (const char *)&(ovndb->nextId),
			       sizeof(int64_t), str_node, strlen(str_node));

	leveldb_write(ovndb->db, ovndb->writeoptions, wb, &errptr);
	leveldb_writebatch_destroy(wb);
	ovndb->nextId++;
	free((char *)str_node);

	if (errptr) {
		printf("\n%s", errptr);
		exit(1);
	}
	return id;

}

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

//the returned value must be copied, it is temporary
json_t *ovndb_retrieve_node(ovndb_t * ovndb, int64_t id)
{
	char *errptr;
	size_t vallen = 0xffffffffffffffff;

	const char *tempval =
	    leveldb_get(ovndb->db, ovndb->readoptions, (const char *)&id,
			sizeof(int64_t),
			&vallen, &errptr);

	json_error_t jerror;
	if (tempval) {
		json_t *node = json_loadb(tempval, vallen, 0, &jerror);
		if (!node) {
			printf("ovndb_retrieve json error:%s", jerror.text);
		}
		free((void *)tempval);

		return node;
	} else {

		return NULL;
	}

}
