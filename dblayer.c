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
MYSQL *conn = NULL;
char *db_server = NULL;
char *db_user = NULL;
char *db_password = NULL; 
char *db_database = "nada";

int db_set_dbserver(char *srv) {
	db_server = malloc(strlen(srv) + 1);
	if (db_server == NULL)
		return ERROR;
	strcpy(db_server,srv);
	return OK;
}

int db_set_sazonality(int saz) {
	sazonality = saz;
	return OK;
}

int db_set_dbuser(char *usr) {
	db_user = malloc(strlen(usr) + 1);
	if (db_user == NULL)
		return ERROR;
	strcpy(db_user,usr);
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

int db_insert_command_line(char *command_line) {

	char *query;
	char *prot_host_name;
	char *prot_service_description;
	MYSQL_RES *result = NULL;
	MYSQL_ROW row;
	int id = 0;

	prot_host_name = escape_string(host_name);
	prot_service_description = escape_string(service_description);

	asprintf(&query,"insert into commands values ('','%s','%s','%s')", command_line,
	                                                                   prot_host_name,
	                                                                   prot_service_description
	);
	do_query(query, FALSE, NULL);
	free(query);
	query = NULL;

	asprintf(&query,"select LAST_INSERT_ID()");
	do_query(query, TRUE, &result);
	while ( (row = mysql_fetch_row(result)) ) {
		id = atoi(row[0]);
	}
	mysql_free_result(result);
	free(query);
	
	return id;

}

int db_get_command_line_id(char *command_line) {

	int id;
	char *query = NULL;
	char *prot_host_name;
	char *prot_service_description;
	MYSQL_RES *result = NULL;
	MYSQL_ROW row;

	prot_host_name = escape_string(host_name);
	prot_service_description = escape_string(service_description);

	asprintf(&query, "select id from commands where command_line='%s' and host_name='%s' and service_description = '%s'", command_line,
	                                                                                                                      prot_host_name,
	                                                                                                                      prot_service_description
	);

	id = 0;
	do_query(query, TRUE, &result);
	while ( (row = mysql_fetch_row(result)) ) {
		id = atoi(row[0]);
	}

	mysql_free_result(result);
	free(query);

	return id;

}

int db_insert_metric(char *command_line, struct metric_t *mt) {

	char *query = NULL;
	char *prot_cmd_line = NULL;
	char *prot_metric_name = NULL;
	int ret;
	int command_line_id;

	prot_cmd_line = escape_string(command_line);
	prot_metric_name = escape_string(mt->name);

	command_line_id = db_get_command_line_id(prot_cmd_line);
	if (command_line_id == 0) {
		// a new command line
		command_line_id = db_insert_command_line(prot_cmd_line);
	}

	asprintf( &query,
	          "insert into history values( %d, now(), %f, '%s');",
	          command_line_id,
	          mt->value,
	          prot_metric_name
	);

	ret = do_query(query, FALSE, NULL);

	free(prot_cmd_line);
	free(prot_metric_name);
	free(query);

	if (ret != OK)
		return ret;

	return OK;
}

int db_retrieve_last_values(char *command_line, struct metric_t *mt, float *last_values) {

	int i = 0;
	char *prot_cmd_line = NULL;
	char *prot_metric_name = NULL;
	char *query = NULL;
	char *time_gaps = NULL;
	MYSQL_RES *result = NULL;
	MYSQL_ROW row;
	int command_line_id = 0;

	prot_cmd_line = escape_string(command_line);
	prot_metric_name = escape_string(mt->name);

	command_line_id = db_get_command_line_id(prot_cmd_line);

	time_gaps = db_create_time_gaps();

	asprintf( &query,
	          "select value from history where command_line_id=%d and metric='%s' and (%s) order by entry_time asc limit %d",
	          command_line_id,
	          prot_metric_name,
	          time_gaps,
	          max_entries
	);

	do_query(query, TRUE, &result);
	while ( (row = mysql_fetch_row(result)) ) {
		*(last_values + i) = atof(row[0]);
		i++;
	}
	mysql_free_result(result);

	free(query);
	free(prot_cmd_line);
	free(prot_metric_name);
	free(time_gaps);

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

		if ((time_begin = malloc(sizeof(struct tm))) == NULL) {
			free(str_time_begin);
			free(str_time_end);
			return NULL;
		}

		if ((time_end = malloc(sizeof(struct tm))) == NULL) {
			free(str_time_begin);
			free(str_time_end);
			return NULL;
		}

		localtime_r(&before, time_begin);
		localtime_r(&after, time_end);

		strftime(str_time_begin, 20, "%Y-%m-%d %H:%M:%S", time_begin);
		strftime(str_time_end, 20, "%Y-%m-%d %H:%M:%S", time_end);

		asprintf(&tmp_time_gap," (entry_time between '%s' and '%s') ", str_time_begin, str_time_end);

		if (i > 0) {
			time_gaps = realloc(time_gaps, strlen(time_gaps) + 3);
			strcat(time_gaps,"or");
		}
		
		time_gaps = realloc(time_gaps, strlen(time_gaps) + strlen(tmp_time_gap) + 1);
		strcat(time_gaps,tmp_time_gap);

		free(time_begin);
		free(time_end);
		free(tmp_time_gap);
		tmp_time_gap = NULL;


	}

	free(str_time_begin);
	free(str_time_end);

	return time_gaps; 

}

int db_open_conn() {

	conn = mysql_init(NULL);
	if ( !mysql_real_connect(conn, db_server, db_user, db_password, db_database, 0, NULL, 0)) {
		return ERROR;
	}

	return OK;
}

void db_close_conn() {
	mysql_close(conn);
	free(db_server);
	free(db_user);
	free(db_password);
}

char *escape_string(char *from) {

	char *to = NULL;

	to = malloc( strlen(from) * 2 + 1);
	if (to == NULL)
		return NULL;
	mysql_real_escape_string(conn, to, from, strlen(from));

	return to; 
}

int do_query(char *q, int return_values, MYSQL_RES **result) {

	MYSQL_RES *res = NULL;

	if (mysql_query(conn, q)) {
		return ERROR;
	}

	if (return_values) {
		res = mysql_store_result(conn);
		*result = res;
	}
	

	return OK;

}
