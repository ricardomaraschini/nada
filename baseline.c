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
 * Compile: 
 * $  make all
 *
 * Execute:
 * $  ./nada /path/to/a/nagios/plugin/with/perfdata
 *
 */
#include "baseline.h"

int main(int argc, char *argv[]) {

	FILE *command = NULL;
	char *command_line = NULL;
	char *line = NULL;
	char *line_bkp = NULL;
	char *baseline_perfdata = NULL;
	char *baseline_perfdata_aux = NULL;
	char *ini_path;
	char *aux = NULL;
	struct metric_t *mt;
	struct metric_t *aux_mt;
	struct metric_t *metrics_root;
	struct deviation_t *deviation;
	int exit_code = OK;
	int ret = 0;
	int min_entries = 0;
	int collected_entries = 0;
	int allow_negatives = TRUE;
	float tolerance = 0;
	unsigned int name_last_pos = 0;
	dictionary *ini;
	extern char **environ;

	if (argc <= 1) {
		printf("Invalid check command supplied\n");
		return UNKNOWN;
	}

	// search for hostname and servicedescription macros
	line = *(environ + ret++);
	while(line) {
		aux = strtok(line,"=");
		if (!strcmp(aux,"NAGIOS_HOSTNAME")) { 
			host_name = strtok(NULL,"=");
		} else if (!strcmp(aux,"NAGIOS_SERVICEDESC")) {
			service_description = strtok(NULL,"=");
		}
		ret++;
		line = *(environ + ret);
	}
	line = NULL;

	if (!host_name || !service_description) {
		printf("Unable to locate NAGIOS_HOSTNAME and NAGIOS_SERVICEDESC macros\n");
		return UNKNOWN;
	}

	asprintf(&ini_path,"%s/baseline.ini",INSTALLPATH);

	ini = iniparser_load(ini_path);
	if (ini == NULL) {
		printf("Unable to process %s file\n",ini_path);
		return UNKNOWN;
	}

	// i personally dont like to use 'extern' variables 
	// with this approach we can easily provide more backends support
	aux = iniparser_getstring(ini, "database:host", NULL);
	ret = (aux) ? db_set_dbserver(aux) : db_set_dbserver("localhost");
	if (ret != OK)
		goto memory_allocation_error;

	aux = iniparser_getstring(ini, "database:user", NULL);
	ret = (aux) ? db_set_dbuser(aux) : db_set_dbuser("root");
	if (ret != OK)
		goto memory_allocation_error;

	aux = iniparser_getstring(ini, "database:password", NULL);
	ret = (aux) ? db_set_dbpassword(aux) : db_set_dbpassword("");
	if (ret != OK)
		goto memory_allocation_error;

	aux = iniparser_getstring(ini, "general:baselinealgorithm", "standard_deviation");
	baseline_algorithm = STANDARDDEVIATION; 
	if (strstr(aux,"exponential_smoothing"))
		baseline_algorithm = EXPONENTIALSMOOTH;

	aux = iniparser_getstring(ini, "database:dbname", NULL);
	ret = (aux) ? db_set_dbname(aux) : db_set_dbname("nada");
	if (ret != OK)
		goto memory_allocation_error;

	db_set_max_entries( iniparser_getint(ini, "general:maxentries", MAXENTRIESTODEVIATION) );
	db_set_sazonality( iniparser_getint(ini, "general:sazonality", SAZONALITY) );
	min_entries = iniparser_getint(ini, "general:minentries", MINENTRIESTODEVIATION);
	allow_negatives = iniparser_getboolean(ini, "general:allownegatives", TRUE);
	tolerance = (float)iniparser_getint(ini, "general:tolerance", DEVIATIONTOLERANCE) / 100 + 1;

	// we no longer need dictionary
	iniparser_freedict(ini);
	free(ini_path);

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

	line = malloc(MAXPLUGINOUTPUT + 1);
	if (line == NULL) {
		free(command_line);
		pclose(command);
		goto memory_allocation_error;
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

	// plugin output returned value becames
	// our default output return code
	exit_code = WEXITSTATUS(pclose(command));

	if ((line_bkp = malloc(strlen(line) + 1)) == NULL) {
		free(command_line);
		free(line);
		goto memory_allocation_error;
	}
	strcpy(line_bkp,line);
	line[strlen(line) - 1] = '\x0';

	strtok(line_bkp,"|");
	metrics_root = parse_perfdata(strtok(NULL,"|"));
	if (metrics_root == NULL) {
		printf("Error parsing metric data - %s\n",line);
		free(command_line);
		free(line_bkp);
		return UNKNOWN;
	}

	ret = db_open_conn();
	if (ret != OK) {
		printf("Unable to connect to database\n");
		free(command_line);
		free(line);
		free(line_bkp);
		return UNKNOWN;
	}

	if ((baseline_perfdata = malloc(1)) == NULL) {
		free(command_line);
		free(line);
		free(line_bkp);
		goto memory_allocation_error;
	}
	*baseline_perfdata = '\x0';
	for(mt=metrics_root; mt != NULL; mt=mt->next) {

		deviation = get_deviation( command_line, 
		                           mt, 
		                           &collected_entries, 
		                           tolerance,
		                           allow_negatives
		);


		if (collected_entries > min_entries) {

			// baseline has been broken
			if (mt->value < deviation->bottom || mt->value > deviation->top) {
				// change return code
				exit_code = CRITICAL;
			}
 
		}

		/* XXX
		 * this certainly is not the more 
		 * appropriate way to do it
		 */
		name_last_pos = strlen(mt->name) - 1;
		if (mt->name[name_last_pos] == '\'') {
			mt->name[name_last_pos] = '\x0';
			asprintf( &baseline_perfdata_aux," %s_top'=%.3f%s;;;; %s_bottom'=%.3f%s;;;; ", 
			                                 mt->name, 
			                                 deviation->top,
			                                 mt->unit, 
			                                 mt->name, 
			                                 deviation->bottom,
			                                 mt->unit
			);

		} else if (mt->name[name_last_pos] == '"') {
			mt->name[name_last_pos] = '\x0';
			asprintf( &baseline_perfdata_aux," %s_top\"=%.3f%s;;;; %s_bottom\"=%.3f%s;;;; ", 
			                                 mt->name, 
			                                 deviation->top,
			                                 mt->unit, 
			                                 mt->name, 
			                                 deviation->bottom,
			                                 mt->unit
			);
		} else {
			asprintf( &baseline_perfdata_aux," %s_top=%.3f%s;;;; %s_bottom=%.3f%s;;;; ", 
			                                 mt->name, 
			                                 deviation->top,
			                                 mt->unit, 
			                                 mt->name, 
			                                 deviation->bottom,
			                                 mt->unit
			);
		}

		baseline_perfdata = realloc(baseline_perfdata, strlen(baseline_perfdata) + strlen(baseline_perfdata_aux) + 1);
		strcat(baseline_perfdata,baseline_perfdata_aux);
		free(baseline_perfdata_aux);

		free(deviation);
	}

	printf( "%s %s\n", line, baseline_perfdata);

	mt = metrics_root;
	while(mt) {
		aux_mt = mt->next;
		free(mt->name);
		free(mt->unit);
		free(mt);
		mt = aux_mt;		
	}

	free(command_line);
	free(line);
	free(line_bkp);
	free(baseline_perfdata);
	db_close_conn();

	return exit_code;

	memory_allocation_error:
	fprintf(stderr,"Memory allocation error\n");
	return CRITICAL;

}

struct deviation_t *get_deviation( char *command_line, 
                                   struct metric_t *mt, 
                                   int *collected_entries, 
                                   float tolerance,
	                           int allow_negatives) {

	struct deviation_t *dv = NULL;
	float deviation = 0;
	float average = 0;
	float aux = 0;
	float tosqrt = 0;
	float sum = 0;
	float smoothed = 0;
	int i = 0;
	int max_entries = MAXENTRIESTODEVIATION;
	float *last_values = NULL;

	max_entries = db_get_max_entries();
	*collected_entries = 0;

	last_values = malloc(sizeof(float) * max_entries); 
	if (last_values == NULL)
		return NULL;

	for(i=0; i<max_entries; i++) {
		*(last_values + i) = (float)-1;
	}

	db_retrieve_last_values(command_line, mt, last_values);
	db_insert_metric(command_line, mt);

	i = 0;
	dv = malloc(sizeof(struct deviation_t));
	if (dv == NULL) {
		free(last_values);
		return NULL;
	}

	if (baseline_algorithm == STANDARDDEVIATION) {

		sum = 0;
		while(*(last_values + i) != -1 && i < max_entries) {
			sum += *(last_values + i);
			i++;
		}

		*collected_entries = i;

		if (i > 0) {

			average = sum / i;

			i = 0;
			while(*(last_values + i) != -1 && i < max_entries) {
				aux = *(last_values + i);
				tosqrt += ( aux - average) * ( aux - average);
				i++;
			}

			if (i > 0) {
				deviation = sqrt((double)tosqrt/i) * tolerance;
			}
		}



		dv->top = average + deviation;
		dv->bottom = average - deviation;

	} else if (baseline_algorithm ==  EXPONENTIALSMOOTH) {

		smoothed = 0;
		while(*(last_values + i) != -1 && i < max_entries) {

			aux = *(last_values + i);

			if (i > 0) {
				smoothed = smoothed + EXPONENTIALALPHA * (aux - smoothed);
			} else {
				smoothed = aux;
			}

			i++;
		}

		dv->top = smoothed * tolerance;
		dv->bottom = smoothed * ( 2 - tolerance);

	}

	if (!allow_negatives && dv->bottom < 0) {
		dv->bottom = (float)0;
	}

	free(last_values);

	return dv;

}

struct metric_t *parse_perfdata(char *perfdata) {

	int ret;
	int i;
	int length;
	char *metric_string = NULL;
	char *aux_string = NULL;
	regex_t regex;
	regex_t metric_regex;
	regmatch_t pmatch[MAXMETRICS];
	struct metric_t *metric = NULL;
	struct metric_t *previous_metric = NULL;

	// regex to split by metric
	// OK: \PhysicalDisk(0 C:)\Disk Transfers/sec: 0|'\PhysicalDisk(0 C:)\Disk Transfers/sec'=0;20;25;
	ret = regcomp(&regex," *([^=]+=[^ ]+)", REG_EXTENDED); 
	if (ret != 0) {
		printf("Unable to compile regular expression\n");
		return NULL;
	}

	// a regex to extract our metric values 
	ret = regcomp(&metric_regex, "([^=]+)=([0-9.]+)([^;]*);*([^;]*);*([^;]*);*([^;]*);*([^;]*)", REG_EXTENDED);
	if (ret != 0) {
		printf("Unable to compile internal regular expression\n");
		return NULL;
	}

	// for every metric, we gonna extract our data
	while(regexec(&regex, perfdata, MAXMETRICS, pmatch, 0) == 0 ) {

		length = pmatch[1].rm_eo - pmatch[1].rm_so + 1;

		// create a string to store our metric data
		metric_string = (char *)calloc(length,sizeof(char));

		// copy metric data to string
		strncpy(metric_string, perfdata + pmatch[1].rm_so,
		    pmatch[1].rm_eo - pmatch[1].rm_so);

		// advance perfdata to the next metric
		perfdata += pmatch[1].rm_eo;

		//printf("%s\n",metric_string);

		// apply the regex to one metric data
		if (regexec(&metric_regex, metric_string, MAXMETRICS, pmatch, 0) == 0) {

			metric = malloc(sizeof(struct metric_t));
			metric->next = NULL;

			// iterate through our matches
			for(i=1; i<=METRICFIELDS; i++) {

				// end of matches
				if (pmatch[i].rm_so < 0)
					break;

				length = pmatch[i].rm_eo - pmatch[i].rm_so + 1;

				aux_string = calloc(length,sizeof(char));
				strncpy(aux_string, 
				    metric_string + pmatch[i].rm_so,
				    pmatch[i].rm_eo - pmatch[i].rm_so);

				switch(i) {

				case METRICNAME:
					metric->name = strdup(aux_string);
					break;
					
				case METRICVALUE:
					metric->value = atof(aux_string);
					break;

				case METRICUNIT:
					metric->unit = strdup(aux_string);
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


				free(aux_string);


			}

			if (previous_metric) {
				metric->next = previous_metric;
			}

			previous_metric = metric;


		}

		free(metric_string);


	}

	regfree(&metric_regex);
	regfree(&regex);

	return metric;

}


char *read_command_line(int argc, char *argv[]) {

	int i;
	char *line = NULL;

	if (asprintf(&line,"%s",argv[1]) < 0)
		return NULL;

	for(i=2; i<argc; i++) {
		line = realloc(line, strlen(argv[i]) + strlen(line) + 2);
		strcat(line," ");
		strcat(line,argv[i]);
	}

	return line;

}
