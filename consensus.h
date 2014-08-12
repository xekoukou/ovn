#ifndef CONSENSUS_H
#define CONSENSUS_H

#include<cassandra.h>
#include<czmq.h>
#include"lib_sha512/sha512.h"
#include"consensus.pb-c.h"

#define SAVE_INTERVAL 200

struct consensus_t {
	CassCluster *cluster;
	CassSession *session;
	int64_t ordered_id;
	int32_t set_id;
	char hist_id[SHA512_LENGTH];
	const CassPrepared *prepared[3];
	int save_iter;
};

typedef struct consensus_t consensus_t;

struct consensus_request_t {
	zframe_t *address;	//null means that the request was external
	Consensus *con;
	int64_t ordered_id;
	int32_t set_id;
	char hist_id[SHA512_LENGTH];
	int broadcast;		// whether the request needs to be broadcast or not
};

typedef struct consensus_request_t consensus_request_t;

void consensus_init(consensus_t ** consensus, const char *contact_points);

void consensus_close(consensus_t * consensus);

void consensus_get_localdb_uids(consensus_t * consensus);

void consensus_save_uids(consensus_t * consensus);

void consensus_request_init(consensus_request_t ** request, 
			    zmsg_t *msg);

#endif
