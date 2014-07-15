#include"consensus.h"

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

void consensus_init(consensus_t ** consensus_, const char *contact_points[],
		    int numb)
{
	consensus_t *consensus = *consensus_;
	consensus = calloc(1, sizeof(consensus_t));

	CassError rc = 0;
	consensus->cluster = create_cluster(contact_points, numb);

	rc = connect_session(consensus->cluster, &(consensus->session));
	if (rc != CASS_OK) {
		exit(-1);
	}
//initialize prepared statements
	CassString query =
	    cass_string_init
	    ("SELECT parent_id,parent_set_id,parent_hist_id FROM ordered_id.graph WHERE ordered_id = 0 and set_id = 0 and local_id = 0 and local_set_id = 0");
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
	    ("UPDATE ordered_id.graph (parent_id,parent_set_id,parent_hist_id ) VALUES (?,?,?) where ordered_id =0 and set_id =0 and local_id=0 and local_set_id = 0 ");
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

	consensus_get_localdb_uids(consensus);

}

void consensus_get_localdb_uids(consensus_t * consensus)
{
	int rc;
	CassStatement *statement = NULL;
	CassFuture *future = NULL;

	statement =
	    cass_prepared_bind(consensus->prepared[0], 0, CASS_CONSISTENCY_ONE);

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

	int iter = 0;
	while (1) {
		statement =
		    cass_prepared_bind(consensus->prepared[1], 4,
				       CASS_CONSISTENCY_ONE);
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
			}
			if (iter == 0) {
				consensus->ordered_id++;
			} else {
				consensus->set_id++;
			}

			const CassValue *value = cass_row_get_column(row, 0);
			CassString string;
			cass_value_get_string(value, &string);
			memcpy(consensus->hist_id, string.data, string.length);
			cass_result_free(result);

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

		statement =
		    cass_prepared_bind(consensus->prepared[1], 4,
				       CASS_CONSISTENCY_ONE);
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

void consensus_mechanism(consensus_t * consensus, int32_t consensus_type,
			 char *data, size_t size)
{

	char str_sha512[SHA512_LENGTH];
	sha512(data, size, str_sha512);

	char dhash[2 * SHA512_LENGTH];
	memcpy(dhash, str_sha512, SHA512_LENGTH);
	memcpy(dhash + SHA512_LENGTH, consensus->hist_id, SHA512_LENGTH);
	char new_hist_id[SHA512_LENGTH];
	sha512(dhash, 2 * SHA512_LENGTH, new_hist_id);

	switch (consensus_type) {
	case 0:
//TODO need to send to others first and verify they got it 
		consensus->set_id++;
		memcpy(consensus->hist_id, new_hist_id, SHA512_LENGTH);

		break;

	}

}

int main(int argc, char **argv)
{

	if (argc != 3) {
		printf
		    ("\nPlease provide the ip address and port for the server to bind\nAlso one contact point for the cassandra cluster");
		exit(1);
	}

	consensus_t *consensus;
	consensus_init(&consensus, (const char **)&(argv[3]), 1);

	//create the server sockets
	zctx_t *ctx = zctx_new();
	void *router = zsocket_new(ctx, ZMQ_ROUTER);
	int port = atoi(argv[2]);
	int rc = zsocket_bind(router, "tcp://%s:%d", argv[1], port);
	if (rc != 0) {
		printf("The consensus_server could't bind to %s:%d", argv[1],
		       port);
		exit(-1);
	}
// TODO in the future we will need a poll with the socket of the external consensus mechanism

	while (1) {
		zmsg_t *msg = zmsg_recv(router);
		if (msg) {

			zframe_t *address = zmsg_unwrap(msg);

			int32_t consensus_type;

			memcpy(&consensus_type, zframe_data(zmsg_first(msg)),
			       sizeof(int32_t));
			zframe_t *fdata = zmsg_next(msg);
			char *data = (char *)zframe_data(fdata);
			size_t size = zframe_size(fdata);
			zmsg_destroy(&msg);

			consensus_mechanism(consensus, consensus_type, data,
					    size);
			zmsg_t *response = zmsg_new();
			zmsg_addmem(response, &consensus_type, sizeof(int32_t));
			zmsg_addmem(response, &(consensus->ordered_id),
				    sizeof(int64_t));
			zmsg_addmem(response, &(consensus->set_id),
				    sizeof(int32_t));
			zmsg_addmem(response, consensus->hist_id,
				    SHA512_LENGTH);

			zmsg_wrap(response, address);
			zmsg_send(&response, router);

			consensus_save_uids(consensus);

		}

	}

}
