#include"ovn_server.h"

/*
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

*/

void process_newNodeRequest(ovndb_t * ovndb,
			    new_node_request_t * server_request,
			    server_requests_t * server_requests)
{

	switch (server_request->state) {

	case 0:;
		server_request->requests = calloc(sizeof(db_new_node_t *), 1);
		server_request->requests[0] = calloc(sizeof(db_new_node_t), 1);
		server_request->id =
		    ovndb_insert_node(ovndb, server_request->requests[0],
				      &(server_request->set_id),
				      server_request->hid);
		server_request->consensus_req++;
		server_request->state = ST_NEW_NODE_CONSENSUS_RESPONSE;
		break;
	case ST_NEW_NODE_CONSENSUS_RESPONSE:;
		server_request->consensus_req--;
		server_request->requests[0]->consensus_msg =
		    server_request->consensus_msg;
		server_request->id =
		    ovndb_insert_node(ovndb, server_request->requests[0],
				      &(server_request->set_id),
				      server_request->hid);
		server_request->conq++;

		server_request->state = ST_NEW_NODE_SAVED;
		break;

	case ST_NEW_NODE_SAVED:;

		json_t *response = json_object();
		json_object_set_new(response, "type",
				    json_string("newNodeResponse"));
		json_object_set_new(response, "id",
				    json_integer(server_request->id));
		json_object_set_new(response, "set_id",
				    json_integer(server_request->set_id));
		json_object_set_new(response, "ancestorId",
				    json_stringn(server_request->hid,
						 SHA512_LENGTH));
		//here we will clean the memory, not pass info to set_id, hid, id
		ovndb_insert_node(ovndb, server_request->requests[0],
				  &(server_request->set_id),
				  server_request->hid);

		free(server_request->requests[server_request->conq - 1]);
		server_request->conq--;

		free(server_request->requests);

		process_response(response,
				 (server_request_t *) server_request,
				 server_requests);

	}
}

/*
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

*/

void process_retrieveRequest(ovndb_t * ovndb,
			     retrieve_request_t * server_request,
			     server_requests_t * server_requests)
{
	switch (server_request->state) {

	case 0:;
		json_t *request =
		    json_object_get(server_request->request, "request");
		json_t *ids = json_object_get(request, "idArray");
		json_t *ancestorIds =
		    json_object_get(request, "ancestorIdArray");
		int64_t size = json_array_size(ids);
		assert(size == json_array_size(ancestorIds));

		server_request->requests =
		    malloc(sizeof(db_retrieve_node_t *) * size);
		server_request->nodeArray = json_array();

		if (size == 0) {
			server_request->state = ST_RETRIEVE_RESPONSE;
			process_retrieveRequest(ovndb,
						server_request,
						server_requests);
			return;
		}
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

	case ST_RETRIEVE_GET_NODE:;
		json_t *node = ovndb_retrieve_node(ovndb,
						   server_request->requests
						   [server_request->conq - 1]);
		json_array_append_new(server_request->nodeArray, node);
		free(server_request->requests[server_request->conq - 1]);
		server_request->conq--;

		if (server_request->conq != 0) {
			break;
		}

	case ST_RETRIEVE_RESPONSE:;

		json_t *response = json_object();
		json_object_set_new(response, "type",
				    json_string("retrieveResponse"));
		json_object_set_new(response, "nodeArray",
				    server_request->nodeArray);

		free(server_request->requests);
		process_response(response,
				 (server_request_t *) server_request,
				 server_requests);

	}
}

void process_request(server_requests_t * server_requests, ovndb_t * ovndb,
		     zmsg_t * msg)
{
	server_requests->nrequests++;
	server_request_t **server_request =
	    &(server_requests->request[server_requests->bottom]);
	server_requests->bottom++;
	if (server_requests->bottom == NUM_CONCURRENT_REQUESTS) {
		server_requests->bottom = 0;
	}

	zframe_t *address = zmsg_unwrap(msg);
	printf("\novn received: %s\n", (const char *)
	       zframe_data(zmsg_first(msg)));
	const char *data;
	size_t data_size = zframe_size(zmsg_first(msg));
	data = (const char *)zframe_data(zmsg_first(msg));
	json_error_t error;
	json_t *request = json_loadb(data, data_size, 0, &error);
	zmsg_destroy(&msg);

	int serviceRequest =
	    json_integer_value(json_object_get(request, "serviceRequest"));

	const char *type;

	if (serviceRequest) {
		type = json_string_value(json_object_get
					 (json_object_get(request, "request"),
					  "type"));
		if (strcmp(type, "retrieveRequest") == 0) {
			*server_request = calloc(sizeof(retrieve_request_t), 1);
			(*server_request)->address = address;
			(*server_request)->request = request;
			(*server_request)->type = REQ_RETRIEVE_REQUEST;
			process_retrieveRequest(ovndb, (retrieve_request_t *)
						* server_request,
						server_requests);
		}		/* else if (strcmp(type, "delNode") == 0)
				   process_delete(ovndb, request);   */
	} else {
		type =
		    json_string_value(json_object_get
				      (json_object_get
				       (json_object_get
					(json_object_get(request, "request"),
					 "data"), "data"), "type"));

		if (strcmp(type, "recipeNewNode") == 0) {
			*server_request = calloc(sizeof(new_node_request_t), 1);
			(*server_request)->address = address;
			(*server_request)->request = request;
			(*server_request)->type = REQ_NEW_NODE;
			process_newNodeRequest(ovndb,
					       (new_node_request_t *) *
					       server_request, server_requests);
		}
	}
	/* else if (strcmp(type, "newLink") == 0)
	   process_newLinkRequest(ovndb, request);
	   else if (strcmp(type, "delLink") == 0)
	   process_delLinkRequest(ovndb, request);
	   else if (strcmp(type, "newNodeData") == 0)
	   process_newNodeDataRequest(ovndb, request);
	   else if (strcmp(type, "newLinkData") == 0)
	   process_newLinkDataRequest(ovndb, request);
	 */
}

void process_consensus_response(server_requests_t * server_requests,
				ovndb_t * ovndb, zmsg_t * msg)
{
	server_request_t *server_request =
	    server_requests->request[server_requests->bottom];

	int32_t consensus_type;
	memcpy(&consensus_type, zframe_data(zmsg_first(msg)), sizeof(int32_t));

	switch (consensus_type) {

	case CONSENSUS_TYPE_GRAPH_NODE:
		server_request->consensus_msg = msg;
		process_newNodeRequest(ovndb,
				       (new_node_request_t *) server_request,
				       server_requests);
		break;
	}

}

void process_response(json_t * response, server_request_t * server_request,
		      server_requests_t * server_requests)
{

	if (response) {
		json_t *response_json = json_object();
		json_object_set(response_json,
				"requestId",
				json_object_get(server_request->request,
						"requestId"));
		json_object_set_new(response_json, "response", response);
		zmsg_t *res = zmsg_new();
		char *res_json_str = json_dumps(response_json, JSON_COMPACT);
		printf("\novn sent: %s\n", res_json_str);
		zmsg_addstr(res, res_json_str);
		free(res_json_str);
		json_decref(response_json);
		zmsg_wrap(res, server_request->address);
		zmsg_send(&res, server_requests->router);
	}

	json_decref(server_request->request);
	free(server_request);

	server_requests->nrequests--;
	server_requests->top++;
	if (server_requests->top == NUM_CONCURRENT_REQUESTS) {
		server_requests->top = 0;
	}

}

void ovndb_response(ovndb_t * ovndb, server_request_t * server_request,
		    server_requests_t * server_requests)
{

	switch (server_request->type) {

	case REQ_RETRIEVE_REQUEST:
		process_retrieveRequest(ovndb,
					(retrieve_request_t *) server_request,
					server_requests);
		break;
	case REQ_NEW_NODE:
		process_newNodeRequest(ovndb,
				       (new_node_request_t *) server_request,
				       server_requests);
		break;
	default:
		exit(-1);
	}

}

int main(int argc, char *argv[])
{

	if (argc != 6) {
		printf
		    ("\nPlease provide the ip address and port for the server to connect\nAlso one contact point for the cassandra cluster and \n the ip and port for the consensus server");
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

	void *consensus_req_socket = zsocket_new(ctx, ZMQ_DEALER);
	port = atoi(argv[5]);
	rc = zsocket_connect(consensus_req_socket, "tcp://%s:%d", argv[4],
			     port);
	if (rc != 0) {
		printf
		    ("The ovn_server could't connect to %s:%d", argv[4], port);
		exit(-1);
	}
	printf("Initializing database client\n");
	//initialize the database
	ovndb_t *ovndb;
	ovndb_init(&ovndb, (const char *)argv[3], consensus_req_socket);
	server_requests_t server_requests = { 0 };
	server_requests.router = router;

	printf("Waiting for requests\n");

	while (1) {
		if (server_requests.nrequests == 0) {
			zmsg_t *msg = zmsg_recv(router);
			process_request(&server_requests, ovndb, msg);
		} else {
//last request
			server_request_t *server_request =
			    server_requests.request[server_requests.top];
			db_request_t *db_request =
			    server_request->requests[server_request->conq - 1];
			if (server_requests.nrequests < NUM_CONCURRENT_REQUESTS) {
				zmsg_t *msg = zmsg_recv_nowait(router);
				if (msg)
					process_request(&server_requests, ovndb,
							msg);
				else {
					if (server_request->consensus_req) {
						zmsg_t *msg =
						    zmsg_recv
						    (ovndb->
						     consensus_req_socket);
						process_consensus_response
						    (&server_requests, ovndb,
						     msg);
					} else {
						bool ready =
						    cass_future_ready
						    (db_request->future
						     [db_request->conq - 1]);
						if (ready)
							ovndb_response(ovndb,
								       server_request,
								       &server_requests);
						else
							zclock_sleep(SLEEP_MS);
					}
				}
			} else {
				if (server_request->consensus_req) {
					zmsg_t *msg =
					    zmsg_recv
					    (ovndb->consensus_req_socket);
					process_consensus_response
					    (&server_requests, ovndb, msg);

				} else {
					cass_future_wait(db_request->future
							 [db_request->conq -
							  1]);
					ovndb_response(ovndb, server_request,
						       &server_requests);
				}
			}
		}
	}

//at the end    
	ovndb_close(ovndb);
}
