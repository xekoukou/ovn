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

#include <cassandra.h>
#include<string.h>
#include<stdlib.h>
#include<stdio.h>

#define NUM_CONCURRENT_REQUESTS 4096

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

CassError execute_query(CassSession * session, const char *query)
{
	CassError rc = 0;
	CassFuture *future = NULL;
	CassStatement *statement =
	    cass_statement_new(cass_string_init(query), 0);

	future = cass_session_execute(session, statement);
	cass_future_wait(future);

	rc = cass_future_error_code(future);
	if (rc != CASS_OK) {
		print_error(future);
		exit(-1);
	}

	cass_future_free(future);
	cass_statement_free(statement);

	return rc;
}

int main(int argc, char *argv[])
{

	if (argc != 2) {
		printf("Please provide the location of a cassandra node\n");
		exit(-1);

	}

	CassError rc = 0;
	CassCluster *cluster = create_cluster((const char *)argv[1]);
	CassSession *session = NULL;
	CassFuture *close_future = NULL;

	rc = connect_session(cluster, &session);
	if (rc != CASS_OK) {
		return -1;
	}

	if (session) {

		printf("Creating position_id keyspace\n");
		execute_query(session,
			      "CREATE KEYSPACE position_id WITH REPLICATION = { 'class' : 'NetworkTopologyStrategy', 'datacenter1' : 3 };");

		printf("Creating ordered_id keyspace\n");
		execute_query(session,
			      "CREATE KEYSPACE ordered_id WITH REPLICATION = { 'class' : 'NetworkTopologyStrategy', 'datacenter1' : 3 };");

//lpos are the first 16 bits *magic number
		printf("Creating position_id.ids table\n");
		execute_query(session,
			      "CREATE TABLE IF NOT EXISTS position_id.ids( \
                                 posX bigint,   \
                                 posY bigint, \
                                 lposX bigint, \
                                 lposY bigint,   \
                                ordered_id bigint,   \
                                set_id int,                  \
                                hist_id varchar,  \
                                PRIMARY KEY ((posX,posY),lposX,lposY))");

		printf("Creating ordered_id.graph table\n");
		execute_query(session,
			      "CREATE TABLE IF NOT EXISTS ordered_id.graph(  \
                                ordered_id bigint,                  \
                                set_id int,                  \
                                local_id bigint,         \
                                local_set_id int,                  \
                                hist_id varchar,  \
                                last_id bigint STATIC,  \
                                last_set_id int STATIC,  \
                                node varchar,   \
                                parent_id bigint STATIC,       \
                                parent_set_id int STATIC,       \
                                parent_hist_id varchar STATIC,   \
                                PRIMARY KEY ((ordered_id,set_id),local_id,local_set_id))");

		printf("Creating ordered_id.node table\n");
		execute_query(session,
			      "CREATE TABLE IF NOT EXISTS ordered_id.node(  \
                                ordered_id bigint,                  \
                                set_id int,                  \
                                local_id bigint,       \
                                local_set_id int,                  \
                                hist_id varchar,  \
                                last_id bigint STATIC,  \
                                last_set_id int STATIC,  \
                                node_summary varchar,   \
                                node_content varchar, \
                                parent_id bigint STATIC,       \
                                parent_set_id int STATIC,       \
                                parent_hist_id varchar STATIC,   \
                                PRIMARY KEY ((ordered_id,set_id),local_id,local_set_id))");

		printf("Creating ordered_id.link table\n");
		execute_query(session,
			      "CREATE TABLE IF NOT EXISTS ordered_id.link(  \
                                ordered_id bigint,                  \
                                set_id int,                  \
                                local_id bigint,       \
                                local_set_id int,                  \
                                hist_id varchar,  \
                                last_id bigint STATIC,  \
                                last_set_id int STATIC,  \
                                link_summary varchar,   \
                                link_content varchar,   \
                                parent_id bigint STATIC,       \
                                parent_set_id int STATIC,       \
                                parent_hist_id varchar STATIC,  \
                                PRIMARY KEY ((ordered_id,set_id),local_id,local_set_id))");

		printf("Inserting initial ledger to the database\n");
		execute_query(session,
			      "INSERT INTO ordered_id.graph (ordered_id, set_id,local_id,local_set_id, parent_id,parent_set_id,parent_hist_id)   \
                      VALUES (0,0,0,0,0,0,'cf83e1357eefb8bdf1542850d66d8007d620e4050b5715dc83f4a921d36ce9ce47d0d13c5d85f2b0ff8318d2877eec2f63b931bd47417a81a538327af927da3e')");

	}
	printf("The databases has been created\n Enjoy!\n");

	close_future = cass_session_close(session);
	cass_future_wait(close_future);
	cass_cluster_free(cluster);

	return 0;

}
