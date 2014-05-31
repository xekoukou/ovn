#include"ovndb.h"
#include<czmq.h>
#include<jansson.h>

json_t *process_newNodeDataRequest(ovndb_t * ovndb, json_t * request)
{
	json_t *new_node_data = json_object_get(request, "nodeData");
	int64_t id = json_integer_value(json_object_get(request, "id"));

	int ok = ovndb_new_node_data(ovndb, id, new_node_data);
	json_t *response = json_object();
	if (ok) {
		json_object_set_new(response, "type",
				    json_string("newNodeData"));
		json_object_set_new(response, "ack", json_string("ok"));

	} else {
		json_object_set_new(response, "type",
				    json_string("newNodeData"));
		json_object_set_new(response, "ack", json_string("fail"));
	}
	return response;
}

json_t *process_delLinkRequest(ovndb_t * ovndb, json_t * request)
{
	json_t *link = json_object_get(request, "link");

	int ok = ovndb_delete_link(ovndb, link);
	json_t *response = json_object();
	json_object_set_new(response, "type", json_string("delLinkResponse"));
	if (ok) {
		json_object_set_new(response, "ack", json_string("ok"));
	} else {
		json_object_set_new(response, "ack", json_string("fail"));

	}
	return response;

}

json_t *process_newLinkRequest(ovndb_t * ovndb, json_t * request)
{
	json_t *link = json_object_get(request, "link");

	int64_t id = ovndb_save_link(ovndb, link);
	json_t *response = json_object();
	if (id) {
		json_object_set_new(response, "type",
				    json_string("newLinkResponse"));
		json_object_set_new(response, "id", json_integer(id));
		json_object_set_new(response, "ack", json_string("ok"));

	} else {
		json_object_set_new(response, "type",
				    json_string("newLinkResponse"));
		json_object_set_new(response, "ack", json_string("fail"));
	}
	return response;
}

json_t *process_newNodeRequest(ovndb_t * ovndb, json_t * request)
{

	int64_t id = ovndb_insert_node(ovndb, json_object_get(request, "node"));

	json_t *response = json_object();
	json_object_set_new(response, "type", json_string("newNodeResponse"));
	json_object_set_new(response, "id", json_integer(id));

	return response;
}

json_t *process_delete(ovndb_t * ovndb, json_t * request)
{
	int64_t id = json_integer_value(json_object_get(request, "id"));
	json_t *response = json_object();
	json_object_set_new(response, "type", json_string("delNode"));
	if (ovndb_delete_node(ovndb, id)) {
		json_object_set_new(response, "ack", json_string("ok"));
	} else {
		json_object_set_new(response, "ack", json_string("fail"));
	}

	return response;

}

json_t *process_retrieveRequest(ovndb_t * ovndb, json_t * request)
{

	json_t *ids = json_object_get(request, "idArray");
	int64_t size = json_array_size(ids);
	json_t *nodeArray = json_array();
	int i;
	for (i = 0; i < size; i++) {
		int64_t id = json_integer_value(json_array_get(ids, i));
		json_t *node = ovndb_retrieve_node(ovndb, id);

		json_array_append_new(nodeArray, node);

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
	printf("\novn received: %s\n",
	       (const char *)zframe_data(zmsg_first(msg)));
	const char *data;
	size_t data_size = zframe_size(zmsg_first(msg));
	data = zframe_data(zmsg_first(msg));
	json_error_t error;
	json_t *request_json = json_loadb(data,
					  data_size, 0, &error);
	zmsg_destroy(&msg);
	json_t *request = json_object_get(request_json, "request");
	const char *type = json_string_value(json_object_get(request, "type"));
	json_t *response;
	if (strcmp(type, "retrieveRequest") == 0) {
		response = process_retrieveRequest(ovndb, request);
	} else {

		if (strcmp(type, "delNode") == 0) {

			response = process_delete(ovndb, request);
		} else {

			if (strcmp(type, "newNode") == 0) {
				response =
				    process_newNodeRequest(ovndb, request);
			} else {

				if (strcmp(type, "newLink") == 0) {
					response =
					    process_newLinkRequest(ovndb,
								   request);
				} else {

					if (strcmp(type, "delLink") == 0) {
						response =
						    process_delLinkRequest
						    (ovndb, request);
					} else {

						if (strcmp(type, "newNodeData")
						    == 0) {
							response =
							    process_newNodeDataRequest
							    (ovndb, request);
						}

					}

				}
			}
		}
	}
	if (response) {
		json_t *response_json = json_object();
		json_object_set(response_json, "requestId",
				json_object_get(request_json, "requestId"));
		json_object_set_new(response_json, "response", response);
		zmsg_t *res = zmsg_new();
		char *res_json_str = json_dumps(response_json, JSON_COMPACT);
		printf("\novn sent: %s\n", res_json_str);
		zmsg_addstr(res, res_json_str);
		free(res_json_str);
		json_decref(response_json);
		json_decref(request_json);
		zmsg_wrap(res, address);
		zmsg_send(&res, router);
	}
}

int main(int argc, char *argv[])
{

	if (argc != 3) {
		printf
		    ("\nPlease provide the ip address for the server to connect and the port\n");
		exit(1);
	}
	//create the server sockets 
	zctx_t *ctx = zctx_new();
	void *router = zsocket_new(ctx, ZMQ_ROUTER);
	int port = atoi(argv[2]);
	int rc = zsocket_connect(router, "tcp://%s:%d", argv[1],
				 port);
	if (rc != 0) {
		printf("The ovn_server could't connect to %s:%d",
		       argv[1], port);
		exit(-1);
	}
	//initialize the database
	ovndb_t *ovndb;
	ovndb_init(&ovndb);
	while (1) {
		process_request(router, ovndb);
	}

//at the end    
	ovndb_close(ovndb);
}
