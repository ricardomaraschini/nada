#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <regex.h>

#define OK 0
#define UNKNOWN 3
#define ERROR 1
#define MAXPLUGINOUTPUT 2048
#define MAXMETRICS 10
#define MAXMETRICNAME 256

#define METRICNAME 1 
#define METRICVALUE 2
#define METRICWARNING 3
#define METRICCRITICAL 4
#define METRICMIN 5
#define METRICMAX 6


struct metric_t {
	char *name;
	float value;
	float warning;
	float critical;
	float min;
	float max;
	struct metric_t *next;
};

char *read_command_line(int argc, char *argv[]);
struct metric_t *parse_perfdata(char *perfdata);
