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

MYSQL *conn;
char *server = "localhost";
char *user = "root";
char *password = ""; 
char *database = "nagios_baseline";


int db_insert_metric(char *command_line, struct metric_t *mt) {

	char *query = NULL;
	char *prot_cmd_line = NULL;
	char *prot_metric_name = NULL;
	int ret;

	prot_cmd_line = escape_string(command_line);
	prot_metric_name = escape_string(mt->name);

	asprintf( &query,
	          "insert into history values( '%s', now(), %f, '%s');",
	          prot_cmd_line,
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
	char *query;
	MYSQL_RES *result = NULL;
	MYSQL_ROW row;

	prot_cmd_line = escape_string(command_line);
	prot_metric_name = escape_string(mt->name);

	asprintf( &query,
	          "select value from history where command_line='%s' and metric='%s' order by entry_time desc limit %d",
	          prot_cmd_line,
	          prot_metric_name,
	          MAXDAYSTODEVIATION
	);

	do_query(query, TRUE, &result);
	while ( (row = mysql_fetch_row(result)) ) {
		*(last_values + i) = atof(row[0]);
		i++;
	}
	mysql_free_result(result);

	return OK;

}

int db_open_conn() {

	conn = mysql_init(NULL);
	if ( !mysql_real_connect(conn, server,user, password, database, 0, NULL, 0)) {
		return ERROR;
	}

	return OK;
}

void db_close_conn() {
	mysql_close(conn);
}

char *escape_string(char *from) {

	char *to = NULL;

	to = malloc( strlen(from) * 2 + 1);
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
