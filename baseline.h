#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <regex.h>
#include <mysql/mysql.h>

#define OK 0
#define UNKNOWN 3
#define ERROR 1
#define MAXPLUGINOUTPUT 2048
#define MAXMETRICS 10
#define MAXMETRICNAME 256

#define METRICNAME 1 
#define METRICVALUE 2
#define METRICUNIT 3
#define METRICWARNING 4 
#define METRICCRITICAL 5
#define METRICMIN 6
#define METRICMAX 7


struct metric_t {
	char *name;
	float value;
	char *unit;
	float warning;
	float critical;
	float min;
	float max;
	struct metric_t *next;
};

struct deviation_t {
	float top;
	float bottom;
};

char *read_command_line(int argc, char *argv[]);
struct metric_t *parse_perfdata(char *perfdata);
struct deviation_t *get_deviation(char *command_line, struct metric_t *mt);

int db_insert_metric(char *command_line, struct metric_t *mt);
int open_db_conn();
void close_db_conn();
char *escape_string(char *from);
int do_query(char *q);
