/*
 * Copyright (C) 2012 Ricardo Maraschini 
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * Description:
 * 
 * 
 *
 * Compile: 
 * $ 
 *
 * Execute:
 * $
 */
#include "baseline.h"

int main(int argc, char *argv[]) {

	FILE *command = NULL;
	char *command_line = NULL;
	char *line = NULL;
	struct metric_t *mt;
	struct metric_t *metrics_root;

	if (argc <= 1) {
		printf("Invalid baseline command supplied\n");
		return UNKNOWN;
	}

	command_line = read_command_line(argc,argv);
	if (command_line == NULL) {
		printf("Error getting command line\n");
		return UNKNOWN;
	}

	command = popen(command_line,"r");
	if (!command) {
		printf("Unable to run supplied command\n");
		free(command_line);
		return UNKNOWN;
	}

	line = malloc(MAXPLUGINOUTPUT);
	if (line == NULL) {
		printf("Unable to allocate memory\n");
		free(command_line);
		pclose(command);
		return UNKNOWN;
	}

	if (fgets(line, MAXPLUGINOUTPUT,command) == NULL) {
		printf("Unable to read plugin output\n");
		free(command_line);
		pclose(command);
		free(line);
		return UNKNOWN;
	}

	if (strstr(line,"|") == NULL) {
		printf("%s",line);
		free(command_line);
		pclose(command);
		free(line);
		return OK;
	}

	free(command_line);
	pclose(command);

	strtok(line,"|");
	metrics_root = parse_perfdata(strtok(NULL,"|"));
	if (metrics_root == NULL) {
		printf("Error parsing metric data\n");
		return UNKNOWN;
	}

	for(mt=metrics_root; mt != NULL ;mt = mt->next) {
		printf("+\n");
	}

	free(line);

	return OK;
}

struct metric_t *parse_perfdata(char *perfdata) {

	int ret;
	int i;
	int j;
	int k;
	char *metric_string = NULL;
	char *aux_string = NULL;
	regex_t regex;
	regex_t metric_regex;
	regmatch_t pmatch[MAXMETRICS];
	struct metric_t *metric = NULL;
	struct metric_t *previous_metric = NULL;

	// regex to split by metric
	ret = regcomp(&regex," *([^=]+=[^;]*;[^;]*;[^;]*;[^;]*;[^ ]*)", REG_EXTENDED); 
	if (ret != 0) {
		printf("Unable to compile regular expression\n");
		return NULL;
	}

	// a regex to extract our metric values 
	ret = regcomp(&metric_regex, "([^=]+)=([^;]*);([^;]*);([^;]*);([^;]*);([^;]*)", REG_EXTENDED);
	if (ret != 0) {
		printf("Unable to compile internal regular expression\n");
		return NULL;
	}

	// for every metric, we gonna extract our data
	while(regexec(&regex, perfdata, MAXMETRICS, pmatch, 0) == 0 ) {

		// create a string to store our metric data
		metric_string = realloc(metric_string, pmatch[1].rm_eo - pmatch[1].rm_so + 1);

		// copy metric data to string
		j = 0;
		for(i=pmatch[1].rm_so; i<pmatch[1].rm_eo; i++) {
			*(metric_string + j) = *(perfdata + i);
			j++;
		}
		*(metric_string + j) = '\x0';

		// advance perfdata to the next metric
		perfdata += pmatch[1].rm_eo;

		printf("%s\n",metric_string);

		// apply the regex to one metric data
		if (regexec(&metric_regex, metric_string, MAXMETRICS, pmatch, 0) == 0) {

			metric = malloc(sizeof(struct metric_t));
			metric->next = NULL;

			// iterate through our matches
			for(i=1; i<MAXMETRICS; i++) {

				// end of matches
				if (pmatch[i].rm_so == -1)
					break;

				aux_string = realloc(aux_string, pmatch[i].rm_eo - pmatch[i].rm_so + 1);
				k = 0;
				for(j=pmatch[i].rm_so;j<pmatch[i].rm_eo;j++) {
					*(aux_string + k ) = *(metric_string + j);
					k++;
				}
				*(aux_string + k) = '\x0'; 
				

				switch(i) {

				case METRICNAME:
					metric->name = strdup(aux_string);
					break;
					
				case METRICVALUE:
					metric->value = atof(aux_string);
					break;

				case METRICWARNING:
					metric->warning = atof(aux_string);
					break;

				case METRICCRITICAL:
					metric->critical = atof(aux_string);
					break;

				case METRICMIN:
					metric->min = atof(aux_string);
					break;

				case METRICMAX:
					metric->max = atof(aux_string);
					break;

				}


			}

			if (previous_metric) {
				metric->next = previous_metric;
			}

			previous_metric = metric;


		}


	}

	free(aux_string);
	regfree(&metric_regex);
	regfree(&regex);

	return metric;

}


char *read_command_line(int argc, char *argv[]) {

	int i;
	int ret;
	char *line = NULL;

	ret = asprintf(&line,"%s",argv[1]);
	if (ret < 0) {
		return NULL;
	}

	for(i=2; i<argc; i++) {
		line = realloc(line, strlen(argv[i]) + strlen(line) + 2);
		strcat(line," ");
		strcat(line,argv[i]);
	}

	return line;

}
