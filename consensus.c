#include"consensus.h"

void print_error(CassFuture * future)
{
	CassString message = cass_future_error_message(future);
	fprintf(stderr, "Error: %.*s\n", (int)message.length, message.data);
}

CassCluster *create_cluster(const char *contact_points_)
{
	CassCluster *cluster = cass_cluster_new();
	CassString contact_points = cass_string_init(contact_points_);
	cass_cluster_set_contact_points(cluster, contact_points);

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

void consensus_init(consensus_t ** consensus_, const char *contact_points)
{
	*consensus_ = calloc(1, sizeof(consensus_t));
	consensus_t *consensus = *consensus_;

	CassError rc = 0;
	consensus->cluster = create_cluster(contact_points);

	rc = connect_session(consensus->cluster, &(consensus->session));
	if (rc != CASS_OK) {
		exit(-1);
	}
//initialize prepared statements //TODO node here is unnecessary but drops an error if we use only static collumns
	CassString query =
	    cass_string_init
	    ("SELECT parent_id,parent_set_id,parent_hist_id,node FROM ordered_id.graph WHERE ordered_id = 0 and set_id = 0 and local_id = 0 and local_set_id = 0");
	CassFuture *future = cass_session_prepare(consensus->session, query);
	cass_future_wait(future);

	rc = cass_future_error_code(future);
	if (rc != CASS_OK) {
		print_error(future);
		exit(-1);
	} else {
		consensus->prepared[0] = cass_future_get_prepared(future);
	}

	cass_future_free(future);

	query =
	    cass_string_init
	    ("SELECT hist_id FROM ordered_id.graph WHERE ordered_id = ? and set_id = ? and local_id=? and local_set_id = ? ");
	future = cass_session_prepare(consensus->session, query);
	cass_future_wait(future);

	rc = cass_future_error_code(future);
	if (rc != CASS_OK) {
		print_error(future);
		exit(-1);
	} else {
		consensus->prepared[1] = cass_future_get_prepared(future);
	}

	cass_future_free(future);

	query =
	    cass_string_init
	    ("UPDATE ordered_id.graph SET parent_id=?, parent_set_id=?, parent_hist_id=? WHERE ordered_id =0 and set_id =0 and local_id=0 and local_set_id = 0 ");
	future = cass_session_prepare(consensus->session, query);
	cass_future_wait(future);

	rc = cass_future_error_code(future);
	if (rc != CASS_OK) {
		print_error(future);
		exit(-1);
	} else {
		consensus->prepared[2] = cass_future_get_prepared(future);
	}

	cass_future_free(future);

	printf("Getting localdb_uids.\n");
	consensus_get_localdb_uids(consensus);

}

void consensus_get_localdb_uids(consensus_t * consensus)
{
	int rc;
	CassStatement *statement = NULL;
	CassFuture *future = NULL;

	printf("Getting last ledger.\n");
	statement = cass_prepared_bind(consensus->prepared[0], 0);

	future = cass_session_execute(consensus->session, statement);
	cass_future_wait(future);

	rc = cass_future_error_code(future);
	if (rc != CASS_OK) {
		print_error(future);
		exit(-1);
	} else {
		const CassResult *result = cass_future_get_result(future);
		const CassRow *row = cass_result_first_row(result);
		const CassValue *value = cass_row_get_column(row, 0);
		cass_value_get_int64(value, &(consensus->ordered_id));
		value = cass_row_get_column(row, 1);
		cass_value_get_int32(value, &(consensus->set_id));

		value = cass_row_get_column(row, 2);
		CassString string;
		cass_value_get_string(value, &string);
		memcpy(consensus->hist_id, string.data, string.length);
		cass_result_free(result);
	}

	cass_future_free(future);
	cass_statement_free(statement);

	printf("Checking if the ledger was behind the real transactions.\n");
	int iter = 0;
	consensus->ordered_id++;
	while (consensus->ordered_id) {
		statement = cass_prepared_bind(consensus->prepared[1], 4);
		cass_statement_bind_int64(statement, 0, consensus->ordered_id);
		cass_statement_bind_int32(statement, 1, consensus->set_id);
		cass_statement_bind_int64(statement, 2, consensus->ordered_id);
		cass_statement_bind_int32(statement, 3, consensus->set_id);

		future = cass_session_execute(consensus->session, statement);
		cass_future_wait(future);

		rc = cass_future_error_code(future);
		if (rc != CASS_OK) {
			print_error(future);
			exit(-1);
		} else {
			const CassResult *result =
			    cass_future_get_result(future);
			const CassRow *row = cass_result_first_row(result);
			if (!row) {
				if (iter == 0) {
					consensus->ordered_id--;
					iter = 1;
				} else {
					consensus->set_id--;
					break;
				}
			} else {
				if (iter == 0) {
					consensus->ordered_id++;
					consensus->set_id = 0;
				} else {
					consensus->set_id++;
				}

				const CassValue *value =
				    cass_row_get_column(row, 0);
				CassString string;
				cass_value_get_string(value, &string);
				memcpy(consensus->hist_id, string.data,
				       string.length);
				cass_result_free(result);
			}
		}

		cass_future_free(future);
		cass_statement_free(statement);

	}

}

void consensus_save_uids(consensus_t * consensus)
{
	int rc;
	CassStatement *statement = NULL;
	CassFuture *future = NULL;

	if (consensus->save_iter < SAVE_INTERVAL) {
		consensus->save_iter++;
	} else {

		statement = cass_prepared_bind(consensus->prepared[1], 4);
		cass_statement_bind_int64(statement, 0, consensus->ordered_id);
		cass_statement_bind_int32(statement, 1, consensus->set_id);
		cass_statement_bind_string(statement, 2,
					   cass_string_init2(consensus->hist_id,
							     128));

		future = cass_session_execute(consensus->session, statement);
		cass_future_wait(future);

		rc = cass_future_error_code(future);
		if (rc != CASS_OK) {
			print_error(future);
			exit(-1);
		}

		cass_future_free(future);
		cass_statement_free(statement);

	}

}

//TODO at the moment we just accept the transaction
zframe_t * consensus_mechanism(consensus_t * consensus, consensus_request_t * request)
{
	size_t size = consensus__get_packed_size(request->con);
	uint8_t *buff = malloc(size);
	consensus__pack(request->con, buff);

	char str_sha512[SHA512_LENGTH];
	sha512(buff, size, str_sha512);
	char dhash[2 * SHA512_LENGTH];
	memcpy(dhash, str_sha512, SHA512_LENGTH);
	memcpy(dhash + SHA512_LENGTH, consensus->hist_id, SHA512_LENGTH);
	char new_hist_id[SHA512_LENGTH];
	sha512(dhash, 2 * SHA512_LENGTH, new_hist_id);

        zframe_t *fr_con =zframe_new(buff,size);
	free(buff);

	consensus->ordered_id++;
	consensus->set_id = 0;
	memcpy(consensus->hist_id, new_hist_id, SHA512_LENGTH);

	request->ordered_id = consensus->ordered_id;
	request->set_id = consensus->set_id;
	memcpy(request->hist_id, consensus->hist_id, SHA512_LENGTH);

return fr_con;
}

void consensus_request_init(consensus_request_t ** request_, zmsg_t * msg)
{

	*request_ = calloc(1, sizeof(consensus_request_t));
	consensus_request_t *request = *request_;
	request->address = zmsg_unwrap(msg);
        zframe_t *fr_con=zmsg_first(msg);
	request->con =
	    consensus__unpack(NULL, zframe_size(fr_con),
			      zframe_data(fr_con));
	zmsg_destroy(&msg);
}

int main(int argc, char **argv)
{

	if (argc != 4) {
		printf
		    ("\nPlease provide the ip address and port for the server to bind\nAlso one contact point for the cassandra cluster");
		exit(1);
	}

	consensus_t *consensus;
	consensus_init(&consensus, (const char *)argv[3]);

	//create the server sockets
	zctx_t *ctx = zctx_new();
	void *router = zsocket_new(ctx, ZMQ_ROUTER);
	int port = atoi(argv[2]);
	int rc = zsocket_bind(router, "tcp://%s:%d", argv[1], port);
	if (rc != port) {
		printf("The consensus_server could't bind to %s:%d", argv[1],
		       port);
		exit(-1);
	}
// TODO in the future we will need a poll with the socket of the external consensus mechanism

	while (1) {
//TODO responses must be in the order they were received
		zmsg_t *msg = zmsg_recv(router);
		if (msg) {
			consensus_request_t *request;
			consensus_request_init(&request, msg);
			zframe_t *fr_con=consensus_mechanism(consensus, request);
			zmsg_t *response = zmsg_new();

                        
                        zmsg_push(response,fr_con);
			zmsg_addmem(response, &(request->ordered_id),
				    sizeof(int64_t));
			zmsg_addmem(response, &(request->set_id),
				    sizeof(int32_t));
			zmsg_addmem(response, request->hist_id,
				    SHA512_LENGTH);

			zmsg_wrap(response, request->address);
			zmsg_send(&response, router);
                        consensus__free_unpacked(request->con,NULL);
                        free(request);
			consensus_save_uids(consensus);

		}

	}

}
