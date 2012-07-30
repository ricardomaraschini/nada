#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <regex.h>
#include <math.h>
#include <time.h>
#include <mysql/mysql.h>
#include "iniparser/dictionary.h"
#include "iniparser/iniparser.h"

#define TRUE 1
#define FALSE 0

#define OK 0
#define ERROR 1
#define CRITICAL 2
#define UNKNOWN 3
#define MAXPLUGINOUTPUT 2048
#define MAXMETRICS 10
#define MAXMETRICNAME 256

#define DEVIATIONTOLERANCE 1.05
#define MAXENTRIESTODEVIATION 100 
#define MINENTRIESTODEVIATION 10
#define SAZONALITY 7

#define METRICNAME 1 
#define METRICVALUE 2
#define METRICUNIT 3
#define METRICWARNING 4 
#define METRICCRITICAL 5
#define METRICMIN 6
#define METRICMAX 7 

#define EXPONENTIALALPHA 0.3

#define STANDARDDEVIATION 0
#define EXPONENTIALSMOOTH 1

#ifndef INSTALLPATH
#error Undefined INSTALLPATH
#endif


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
struct deviation_t *get_deviation( char *command_line, 
                                   struct metric_t *mt, 
                                   int *collected_entries, 
                                   float tolerance,
                                   int allow_negatives
);

int baseline_algorithm;
char *host_name;
char *service_description;

int db_insert_metric(char *command_line, struct metric_t *mt);
int db_retrieve_last_values(char *command_line, struct metric_t *mt, float *last_values);
int db_open_conn();
void db_close_conn();
char *db_create_time_gaps();
char *escape_string(char *from);
int do_query(char *q, int return_values, MYSQL_RES **result);
int db_set_dbserver(char *srv);
int db_set_dbpassword(char *pass);
int db_set_dbuser(char *usr);
void db_set_max_entries(int entries);
int db_get_max_entries();
int db_insert_command_line(char *command_line);
int db_get_command_line_id(char *command_line);
int db_set_sazonality(int saz);
