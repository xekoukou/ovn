#include"ovndb.h"
#include<czmq.h>
#include<jansson.h>

json_t *process_insert(ovndb_t * ovndb, json_t * request)
{

	json_t *node = json_object_get(request, "node");
	ovndb_insert_node(ovndb, node);

	//send a response  TODO
	// must return the id of the node because it might have been created

}

json_t *process_delete(ovndb_t * ovndb, json_t * request)
{
	json_t *ids = json_object_get(request, "ids");
	int64_t size = json_array_size(ids);
	int i;
	for (i = 0; i < size; i++) {
		int64_t id = json_integer_value(json_array_get(ids, i));
		ovndb_delete_node(ovndb, id);
	}
	//send a response  TODO

}

json_t *process_retrieve(ovndb_t * ovndb, json_t * request)
{

	json_t *ids = json_object_get(request, "idArray");
	int64_t size = json_array_size(ids);
	json_t *nodeArray = json_array();
	int i;
	for (i = 0; i < size; i++) {
		int64_t id = json_integer_value(json_array_get(ids, i));
		int64_t length;
		char *node_str = ovndb_retrieve_node(ovndb, id, &length);

		json_error_t error;
		json_t *node = json_loads(node_str, 0, &error);
		json_array_append(nodeArray, node);

	}

	json_t *response = json_object();
	json_object_set_new(response, "type", json_string("retrieveResponse"));
	json_object_set_new(response, "nodeArray", nodeArray);

	return response;
}

void process_request(void *router, ovndb_t * ovndb)
{

	zmsg_t *msg = zmsg_recv(router);
	zframe_t *address = zmsg_unwrap(msg);

	json_error_t error;
	json_t *request_json =
	    json_loads((const char *)zframe_data(zmsg_first(msg)), 0, &error);

	json_t *request = json_object_get(request_json, "request");
	const char *type = json_string_value(json_object_get(request, "type"));

	json_t *response;

	if (strcmp(type, "retrieveRequest") == 0) {
		response = process_insert(ovndb, request);

	} else {

		if (strcmp(type, "delete") == 0) {

			response = process_delete(ovndb, request);
		} else {

			if (strcmp(type, "retrieve") == 0) {
				response = process_retrieve(ovndb, response);

			}
		}
	}

	const char *requestId =
	    json_string_value(json_object_get(request_json, "requestId"));

	json_t *response_json = json_object();
	json_object_set_new(response_json, "requestId", json_string(requestId));
	json_object_set_new(response_json, "response", response);

	zmsg_t *res = zmsg_new();
	char *res_json_str = json_dumps(response_json, JSON_COMPACT);
	zmsg_addstr(res, res_json_str);
	free(res_json_str);
	json_decref(response_json);

	zmsg_wrap(res, address);
	zmsg_send(router, &res);
}

int main(int argc, char *argv[])
{

	if (argc != 3) {
		printf
		    ("\nPlease provide the ip address for the server to bind and the port");
		exit(1);
	}
	//create the server sockets 
	zctx_t *ctx = zctx_new();
	void *router = zsocket_new(ctx, ZMQ_ROUTER);
	int port = atoi(argv[2]);
	int rc = zsocket_bind(router, "tcp://%s:%d", argv[1], port);
	if (rc != port) {
		printf("The position_server could't connect to %s:%d", argv[1],
		       port);
	}
	//initialize the database
	ovndb_t *ovndb;
	ovndb_init(&ovndb);

	zpoller_t *poller = zpoller_new(router);
	while (1) {
		void *which = zpoller_wait(poller, -1);
		if (!zpoller_terminated(poller)) {
			return -1;
		}
		if (!zpoller_expired(poller)) {

			if (which == router) {
				process_request(router, ovndb);

			}
		}
	}

//at the end    
	ovndb_close(ovndb);
}
