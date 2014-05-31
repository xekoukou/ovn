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

#include<leveldb/c.h>
#include<string.h>
#include<stdlib.h>
#include<stdio.h>

int main()
{

	char *errptr = NULL;

	leveldb_options_t *options = leveldb_options_create();
	leveldb_options_set_create_if_missing(options, 1);

	leveldb_writeoptions_t *writeoptions = leveldb_writeoptions_create();

	leveldb_writeoptions_set_sync(writeoptions, 1);

	leveldb_t *db = leveldb_open(options, "./ovndb", &errptr);
	if (errptr) {
		printf("\n%s", errptr);
		exit(1);
	}
//set the nextId to 0
	int64_t zero = 0;
	int64_t val_zero = 0;
	size_t vallen = sizeof(int64_t);
	leveldb_put
	    (db, writeoptions, (char *)&zero, sizeof(int64_t),
	     (char *)&val_zero, vallen, &errptr);

	if (errptr) {
		printf("\n%s", errptr);
		exit(1);
	}

	leveldb_close(db);

	printf("The database has been created\n Enjoy!\n");

}
