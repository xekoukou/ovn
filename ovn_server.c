#include"ovndb.h"
#include<czmq.h>

//interface 
//never to be used
struct server_request_t {
	int type;
	int state;		//automaton
	zframe_t *address;
	json_t *request;
	int conq;
	db_requests **requests;

};
typedef struct server_request_t server_request_t;

struct server_requests_t {
	void *router;
	server_request_t *request[NUM_CONCURRENT_REQUESTS];
	int top;
	int bottom;
	int nrequests;
};

typedef struct server_requests_t server_requests_t;

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

json_t *process_newLinkDataRequest(ovndb_t * ovndb, json_t * request)
{
	json_t *link = json_object_get(request, "link");
	int64_t id = 0;
	id = ovndb_save_link(ovndb, link);
	json_t *response = json_object();
	if (id) {
		json_object_set_new(response, "type",
				    json_string("newLinkDataResponse"));
		json_object_set_new(response, "ack", json_string("ok"));

	} else {
		json_object_set_new(response, "type",
				    json_string("newLinkDataResponse"));
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

struct new_node_request_t {
        int type;
        int state;              //automaton
        zframe_t *address;
        json_t *request;
        int conq;
        db_requests *requests[1];

};

typedef struct new_node_request_t new_node_request_t;

#define ST_NEW_NODE_RESPONSE 1

void process_newNodeRequest(ovndb_t * ovndb,
                             new_node_request_t * server_request,server_requests_t *server_requests)
{

switch(server_request->state) {

case 0:
        server_request->requests[0]=calloc(sizeof(db_new_node_t),1);
        server_request->requests[0]->node=json_object_get(server_request->request, "node");
	int64_t id = ovndb_insert_node(ovndb, server_request);
                if (server_request->conq != 0) {
                        break;
                }

case ST_NEW_NODE_RESPONSE:

	json_t *response = json_object();
	json_object_set_new(response, "type", json_string("newNodeResponse"));
	json_object_set_new(response, "id", json_integer(id));

	return response;
}
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

struct retrieve_request_t {
	int type;
	int state;		//automaton
	zframe_t *address;
	json_t *request;
	int conq;
	db_requests **requests;
	json_t *nodeArray;

};
typedef struct retrieve_request_t retrieve_request_t;

#define ST_RETRIEVE_GET_NODE 1
#define ST_RETRIEVE_RESPONSE 2

void process_retrieveRequest(ovndb_t * ovndb,
			     retrieve_request_t * server_request,server_requests_t *server_requests)
{
	switch (server_request->state) {

	case 0:
		json_t * ids =
		    json_object_get(server_request->request, "idArray");
		json_t *ancestorIds =
		    json_object_get(server_request->request, "ancestorIdArray");
		int64_t size = json_array_size(ids);
		assert(size == json_array_size(ancestorIds));

		server_request->requests =
		    malloc(sizeof(db_retrieve_node_t *) * size);
		server_request->nodeArray = json_array();

		int i;
		for (i = 0; i < size; i++) {

			server_request->requests[i] =
			    calloc(sizeof(db_retrieve_node_t), 1);
			server_request->conq++;

			server_request->requests[i]->id =
			    json_integer_value(json_array_get(ids, i));
			server_request->requests[i]->ancestorId =
			    json_integer_value(json_array_get(ancestorIds, i));
			ovndb_retrieve_node(ovndb, server_request->requests[i]);
		}
		server_request->state = ST_RETRIEVE_GET_NODE;
		break;

	case ST_RETRIEVE_GET_NODE:
		json_t * node =
		    ovndb_retrieve_node(ovndb,
					server_request->requests
					[server_request->conq - 1]);
		json_array_append_new(server_request->nodeArray, node);
		free(server_request->requests[server_request->conq - 1]);
		server_request->conq--;

		if (server_request->conq != 0) {
			break;
		}

	case ST_RETRIEVE_RESPONSE:
		free(server_request->requests);
		json_decref(server_request->request);

		json_t *response = json_object();
		json_object_set_new(response, "type",
				    json_string("retrieveResponse"));
		json_object_set_new(response, "nodeArray",
				    server_request->nodeArray);
		process_response(response,
				 server_request->address,server_requests);

	}
}

#define REQ_RETRIEVE_REQUEST 1
#define REQ_NEW_NODE 2

void process_request(server_requests_t * server_requests, ovndb_t * ovndb,
		     zmsg_t * msg)
{
	server_request_t server_request =
	    server_requests->request[server_requests.bottom];
	server_requests->bottom++;
	if (server_requests->bottom == NUM_CONCURRENT_REQUESTS) {
		server_requests->bottom = 0;
	}

	server_request.address = zmsg_unwrap(msg);
	printf("\novn received: %s\n", (const char *)
	       zframe_data(zmsg_first(msg)));
	const char *data;
	size_t data_size = zframe_size(zmsg_first(msg));
	data = zframe_data(zmsg_first(msg));
	json_error_t error;
	json_t *request_json = json_loadb(data, data_size, 0, &error);
	zmsg_destroy(&msg);
	server_request.request = json_object_get(request_json, "request");

	Json_incref(request);
	json_decref(request_json);

	const char *type = json_string_value(json_object_get(request, "type"));
	if (strcmp(type, "retrieveRequest") == 0) {
           server_request = calloc(sizeof(retrieveRequest_data_t),1);
		server_request.type = REQ_RETRIEVE_REQUEST;
		process_retrieveRequest(ovndb,
					(retrieveRequest_data *) &
					server_request, server_requests);
	} else if (strcmp(type, "delNode") == 0)
		process_delete(ovndb, request);
	else if (strcmp(type, "newNode") == 0)
		server_request.type = REQ_NEW_NODE;
		process_newNodeRequest(ovndb, request);
	else if (strcmp(type, "newLink") == 0)
		process_newLinkRequest(ovndb, request);
	else if (strcmp(type, "delLink") == 0)
		process_delLinkRequest(ovndb, request);
	else if (strcmp(type, "newNodeData") == 0)
		process_newNodeDataRequest(ovndb, request);
	else if (strcmp(type, "newLinkData") == 0)
		process_newLinkDataRequest(ovndb, request);

}

void process_response(json_t * response, zframe_t * address, server_requests_t *server_requests)
{
	if (response) {
		json_t *response_json = json_object();
		json_object_set(response_json,
				"requestId",
				json_object_get(request_json, "requestId"));
		json_object_set_new(response_json, "response", response);
		zmsg_t *res = zmsg_new();
		char *res_json_str = json_dumps(response_json, JSON_COMPACT);
		printf("\novn sent: %s\n", res_json_str);
		zmsg_addstr(res, res_json_str);
		free(res_json_str);
		json_decref(response_json);
		zmsg_wrap(res, address);
		zmsg_send(&res, server_requests->router);
	}

free(server_requests->request[server_requests->top]);

server_requests->top++;
if(server_requests->top==NUM_CONCURRENT_REQUESTS){
server_requests->top=0;
}

}

void ovndb_response(ovndb_t * ovndb, server_request_t * server_request)
{

	switch (server_request->type) {

	case REQ_RETRIEVE_REQUEST:
		process_retrieveRequest(ovndb, server_request);
		break;
	default:
		exit(-1);
	}

}

int main(int argc, char *argv[])
{

	if (argc != 4) {
		printf
		    ("\nPlease provide the ip address for the server to connect and the port\nAlso one contact point for the cassandra cluster");
		exit(1);
	}
	//create the server sockets 
	zctx_t *ctx = zctx_new();
	void *router = zsocket_new(ctx, ZMQ_ROUTER);
	int port = atoi(argv[2]);
	int rc = zsocket_connect(router, "tcp://%s:%d",
				 argv[1], port);
	if (rc != 0) {
		printf
		    ("The ovn_server could't connect to %s:%d", argv[1], port);
		exit(-1);
	}
	//initialize the database
	ovndb_t *ovndb;
	ovndb_init(&ovndb, (const char **)&(argv[3]), 1);
	server_requests_t server_requests;

	while (1) {
		if (server_requests.nrequests == 0) {
			zmsg_t *msg = zmsg_recv(router);
			process_request(ovndb, msg);
		} else {
//last request
			server_request_t *server_request =
			    server_requests.request[server_requests.top];
			db_request_t *db_request =
			    server_request.request[server_request.conq - 1];
			if (server_requests.nrequests < NUM_CONCURRENT_REQUESTS) {
				zmsg_t *msg = zmsg_recv_nowait(router);
				if (msg)
					process_request(server_requests, ovndb,
							msg);
				else {
					bool ready =
					    cass_future_ready
					    (db_request.future
					     [db_request.conq - 1]);
					if (ready)
						ovndb_response(ovndb,
							       server_request,
							       request);
					else
						zclock_sleep(SLEEP_MS);
				}
			} else {
				cass_future_wait(db_request.future
						 [db_request.conq - 1]);
				ovndb_response(ovndb, server_request,
					       db_request);
			}
		}
	}

//at the end    
	ovndb_close(ovndb);
}
