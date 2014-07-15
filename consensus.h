#ifndef CONSENSUS_H
#define CONSENSUS_H

#include<cassandra.h>
#include<czmq.h>
#include"lib_sha512/sha512.h"
#include"consensusInProtocol.h"

#define SAVE_INTERVAL 200

struct consensus_t {
	CassCluster *cluster;
	CassSession *session;
	int64_t ordered_id;
	int32_t set_id;
	char hist_id[128];
	const CassPrepared *prepared[3];
	int save_iter;
};

typedef struct consensus_t consensus_t;

void consensus_init(consensus_t ** consensus, const char **contact_points,
		    int numb);

void consensus_close(consensus_t * consensus);

void consensus_get_localdb_uids(consensus_t * consensus);

void consensus_save_uids(consensus_t * consensus);

#endif
