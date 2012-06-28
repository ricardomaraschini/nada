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


	ret = do_query(query);

	free(prot_cmd_line);
	free(prot_metric_name);
	free(query);

	if (ret != OK)
		return ret;

	return OK;
}

int open_db_conn() {

	conn = mysql_init(NULL);
	if ( !mysql_real_connect(conn, server,user, password, database, 0, NULL, 0)) {
		return ERROR;
	}

	return OK;
}

void close_db_conn() {
	mysql_close(conn);
}

char *escape_string(char *from) {

	char *to = NULL;

	to = malloc( strlen(from) * 2 + 1);
	mysql_real_escape_string(conn, to, from, strlen(from));

	return to; 
}

int do_query(char *q) {

	if (mysql_query(conn, q)) {
		return ERROR;
	}

	return OK;

}
