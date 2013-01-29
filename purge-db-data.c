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
 */
#include "baseline.h"

int main(int argc, char *argv[]) {

	char *ini_path;
	char *aux = NULL;
	int exit_code = OK;
	int ret = 0;
	dictionary *ini;


	asprintf(&ini_path,"%s/baseline.ini",INSTALLPATH);

	ini = iniparser_load(ini_path);
	if (ini == NULL) {
		printf("Unable to process %s file\n",ini_path);
		return UNKNOWN;
	}

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

	db_set_max_entries( iniparser_getint(ini, "general:maxentries", MAXENTRIESTODEVIATION) );
	db_set_sazonality( iniparser_getint(ini, "general:sazonality", SAZONALITY) );

	// we no longer need dictionary
	iniparser_freedict(ini);
	free(ini_path);

	ret = db_open_conn();
	if (ret != OK) {
		printf("Unable to connect to mysql database\n");
		return UNKNOWN;
	}

	if (db_purge_old_data() != OK) {
		printf("Unable to purge old data\n");
		exit_code = ERROR;
	}

	db_close_conn();

	return exit_code;

	memory_allocation_error:
		fprintf(stderr,"Memory allocation error\n");
		return CRITICAL;

}
