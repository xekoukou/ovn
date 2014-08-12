#ifndef OVN_SERVER_H
#define OVN_SERVER_H

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
	int consensus_req;	//boolean
	zmsg_t *consensus_msg;
	db_request_t **requests;

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

struct new_node_request_t {
	int type;
	int state;		//automaton
	zframe_t *address;
	json_t *request;
	int conq;
	int consensus_req;	//boolean
	zmsg_t *consensus_msg;
	db_new_node_t **requests;

};

typedef struct new_node_request_t new_node_request_t;

#define ST_NEW_NODE_CONSENSUS_RESPONSE 1
#define ST_NEW_NODE_SAVED 2

struct retrieve_request_t {
	int type;
	int state;		//automaton
	zframe_t *address;
	json_t *request;
	int conq;
	int consensus_req;	//boolean
	zmsg_t *consensus_msg;
	db_retrieve_node_t **requests;
	json_t *nodeArray;

};
typedef struct retrieve_request_t retrieve_request_t;

#define ST_RETRIEVE_GET_NODE 1
#define ST_RETRIEVE_RESPONSE 2

#define REQ_RETRIEVE_REQUEST 1
#define REQ_NEW_NODE 2

void process_response(json_t * response, server_request_t * server_request,
		      server_requests_t * server_requests);

#endif
