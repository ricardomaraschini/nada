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
	char *query = NULL;
	char *time_gaps = NULL;
	MYSQL_RES *result = NULL;
	MYSQL_ROW row;

	prot_cmd_line = escape_string(command_line);
	prot_metric_name = escape_string(mt->name);

	time_gaps = db_create_time_gaps();

	asprintf( &query,
	          "select value from history where command_line='%s' and metric='%s' or (%s) order by entry_time desc limit %d",
	          prot_cmd_line,
	          prot_metric_name,
	          time_gaps,
	          MAXENTRIESTODEVIATION
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

	str_time_begin = malloc(20);
	str_time_end = malloc(20);
	asprintf(&time_gaps," ");

	// date format: 2012-06-27 22:02:55
	for(i=0; i<MAXENTRIESTODEVIATION; i++) {

		previous_week = now - (i * 60 * 60 * 24 * SAZONALITY); // one week

		before = previous_week - time_tolerance;
		after = previous_week + time_tolerance;

		time_begin = malloc(sizeof(struct tm));
		time_end = malloc(sizeof(struct tm));

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
