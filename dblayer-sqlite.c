/*
 * Copyright (C) 2012 Ricardo Maraschini 
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */


#include "baseline.h"

int max_entries;
int sazonality;
sqlite3 *conn = NULL;
char *db_server = NULL;
char *db_user = NULL;
char *db_password = NULL; 
char *db_database = "/tmp/nada";

int db_set_dbserver(char *srv) {
	db_server = malloc(strlen(srv) + 1);
	if (db_server == NULL)
		return ERROR;
	strcpy(db_server,srv);
	return OK;
}

void db_set_sazonality(int saz) {
	sazonality = saz;
}

int db_set_dbuser(char *usr) {
	db_user = malloc(strlen(usr) + 1);
	if (db_user == NULL)
		return ERROR;
	strcpy(db_user,usr);
	return OK;
}

int db_set_dbname(char *dbname) {
	db_database = malloc(strlen(dbname) + 1);
	if (!db_database)
		return ERROR;
	strcpy(db_database, dbname);
	return OK;
}

int db_set_dbpassword(char *pass) {
	db_password = malloc(strlen(pass) + 1);
	if (db_password == NULL)
		return ERROR;
	strcpy(db_password,pass);
	return OK;
}

void db_set_max_entries(int entries) {
	max_entries = entries;
}

int db_get_max_entries() {
	return max_entries;
}

int db_open_conn() {

	char create_commands_table[] = "CREATE TABLE IF NOT EXISTS commands(command_line varchar DEFAULT NULL, host_name varchar, service_description varchar)";
	char create_history_table[] = "CREATE TABLE IF NOT EXISTS history(command_line_id int(11) NOT NULL, entry_time int DEFAULT NULL, value float DEFAULT NULL, metric varchar DEFAULT NULL, FOREIGN KEY (command_line_id) REFERENCES commands(rowid) ON DELETE CASCADE ON UPDATE CASCADE)";
	int res = SQLITE_OK;

	if (sqlite3_open(db_database,&conn))
		return ERROR;

	if (do_query(create_commands_table, FALSE, NULL) != OK)
		return ERROR;

	if (do_query(create_history_table, FALSE, NULL) != OK)
		return ERROR;

	return OK;
}

void db_close_conn() {
	sqlite3_close(conn);
	free(db_server);
	free(db_user);
	free(db_password);
}

int db_retrieve_last_values(char *command_line, struct metric_t *mt, float *last_values) {

	int i = 0;
	char *query = NULL;
	char *time_gaps = NULL;
	sqlite3_stmt *statement = NULL;
	int command_line_id = 0;

	command_line_id = db_get_command_line_id(command_line);
	if (command_line_id == 0) {
		// a new command line
		command_line_id = db_insert_command_line(command_line);
	}

	time_gaps = db_create_time_gaps();

	asprintf( &query,
	          "select value from history where command_line_id=? and metric=? and (%s) order by entry_time asc limit %d",
	          time_gaps,
	          max_entries
	);

	do_query(query, TRUE, &statement);
	sqlite3_bind_int( statement,
	                  1,
	                  command_line_id
	);
	sqlite3_bind_text( statement,
	                   2,
	                   mt->name,
			   strlen(mt->name),
			   NULL
	);

	while(sqlite3_step(statement) == SQLITE_ROW) {
		*(last_values + i) = sqlite3_column_double(statement, 0);
		i++;
	}
	sqlite3_finalize(statement);

	free(query);
	free(time_gaps);

	return OK;
}

int db_get_command_line_id(char *command_line) {

	int id;
	int retval;
	char *query = NULL;
	sqlite3_stmt *result = NULL;

	asprintf(&query, "select rowid from commands where command_line=? and host_name=? and service_description=?");
	do_query(query, TRUE, &result);
	sqlite3_bind_text( result,
	                   1,
	                   command_line,
	                   strlen(command_line),
	                   NULL
	);
	sqlite3_bind_text( result,
	                   2,
	                   host_name,
	                   strlen(host_name),
	                   NULL
	);
	sqlite3_bind_text( result,
	                   3,
	                   service_description,
	                   strlen(service_description),
	                   NULL
	);

	retval = sqlite3_step(result);
	if (retval == SQLITE_DONE) {
		sqlite3_finalize(result);
		return 0;
	}
	
	if (retval == SQLITE_ROW) {
		id = sqlite3_column_int(result,0);
	}

	sqlite3_finalize(result);
	return id;
}


int db_insert_command_line(char *command_line) {

	char *query;
	sqlite3_stmt *statement = NULL;

	asprintf(&query,"insert into commands values(?,?,?)");
	do_query(query, TRUE, &statement);

	sqlite3_bind_text( statement,
	                   1,
	                   command_line,
	                   strlen(command_line),
	                   NULL
	);

	sqlite3_bind_text( statement,
	                   2,
	                   host_name,
	                   strlen(host_name),
	                   NULL
	);

	sqlite3_bind_text( statement,
	                   3,
	                   service_description,
	                   strlen(service_description),
	                   NULL
	);

	sqlite3_step(statement);
	sqlite3_finalize(statement);
	free(query);

	return db_get_command_line_id(command_line);

}

int db_insert_metric(char *command_line, struct metric_t *mt) {

	char *query = NULL;
	sqlite3_stmt *statement;
	int ret;
	int command_line_id;

	command_line_id = db_get_command_line_id(command_line);
	if (command_line_id == 0) {
		// a new command line
		command_line_id = db_insert_command_line(command_line);
	}

	asprintf(&query,"insert into history values( ?, strftime('%%s', 'now'), ?, ?)");
	do_query(query, TRUE, &statement);

	sqlite3_bind_int( statement,
	                  1,
	                  command_line_id
	);
	sqlite3_bind_double( statement,
	                     2,
	                     mt->value 
	);
	sqlite3_bind_text( statement,
	                   3,
	                   mt->name,
	                   strlen(mt->name),
	                   NULL
	);

	sqlite3_step(statement);
	free(query);

	return OK;
}


int do_query(char *q, int return_values, sqlite3_stmt **result) {

	sqlite3_stmt *res = NULL;
	int ret;

	if (return_values) {
		ret = sqlite3_prepare(conn, q, strlen(q), result, NULL);
		if (ret) {
			printf("sqlite error: %s\n", sqlite3_errmsg(conn));
			return ERROR;
		}

		return OK;
	}

	ret = sqlite3_exec(conn, q, 0,0,0);
	if (ret != SQLITE_OK) {
		printf("sqlite error: %s\n", sqlite3_errmsg(conn));
		return ERROR;
	}

	return OK;
}

char *db_create_time_gaps() {

	time_t now;
	time_t previous_week;
	time_t before; 
	time_t after;
	struct tm *time_begin;
	struct tm *time_end;
	char *time_gaps = NULL;
	char *tmp_time_gap = NULL;
	char *str_time_begin = NULL;
	char *str_time_end = NULL;
	int time_tolerance = 300;
	int i;

	now = time(NULL);

	if ((str_time_begin = malloc(20)) == NULL)
		return NULL;

	if ((str_time_end = malloc(20)) == NULL)
		return NULL;

	asprintf(&time_gaps," ");

	// date format: 2012-06-27 22:02:55
	for(i=0; i<max_entries; i++) {

		previous_week = now - (i * 60 * 60 * 24 * sazonality);

		before = previous_week - time_tolerance;
		after = previous_week + time_tolerance;

		asprintf(&tmp_time_gap," (entry_time between %lu and %lu) ", before, after);

		if (i > 0) {
			time_gaps = realloc(time_gaps, strlen(time_gaps) + 3);
			strcat(time_gaps,"or");
		}
		
		time_gaps = realloc(time_gaps, strlen(time_gaps) + strlen(tmp_time_gap) + 1);
		strcat(time_gaps,tmp_time_gap);

		free(tmp_time_gap);
		tmp_time_gap = NULL;

	}

	free(str_time_begin);
	free(str_time_end);

	return time_gaps; 

}

int db_purge_old_data() {

	struct timeval now;
	time_t time_boundary;
	char *query;

	if (gettimeofday(&now,NULL) < 0)
		return ERROR;

	time_boundary = now.tv_sec - max_entries * sazonality * 24 * 60 * 60;
	asprintf(&query, "delete from history where entry_time < %lu", time_boundary);
	if (do_query(query, FALSE, NULL) != OK)
		return ERROR;
		
	free(query);

	return OK;

}
