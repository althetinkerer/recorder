#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <time.h>
#include "json.h"
#include "storage.h"

void usage(char *prog)
{
	printf("Usage: %s [options..] [file ...]\n", prog);
	printf("  --help		-h	this message\n");
	printf("  --list		-l	list users (or a user's (-u) devices\n");
	printf("  --user username	-u	specify username\n");
	printf("  --device devicename   -d	specify device name\n");
	printf("  --from <time>         -F      from date/time\n");
	printf("  --to   <time>         -T      to date/time\n");
	printf("         specify <time> as     YYYY-MM-DDTHH:MM:SS\n");
	printf("                               YYYY-MM-DDTHH:MM\n");
	printf("                               YYYY-MM-DDTHH\n");
	printf("                               YYYY-MM-DD\n");
	printf("                               YYYY-MM\n");

	exit(1);
}

int main(int argc, char **argv)
{
	char *progname = *argv;
	int c;
	int list = 0;
	char *username = NULL, *device = NULL, *time_from = NULL, *time_to = NULL;
	JsonNode *json, *obj, *locs;
	time_t now, s_lo, s_hi;

	time(&now);

	while (1) {
		static struct option long_options[] = {
			{ "help",	no_argument,	0, 	'h'},
			{ "list",	no_argument,	0, 	'l'},
			{ "user",	required_argument, 0, 	'u'},
			{ "device",	required_argument, 0, 	'd'},
			{ "from",	required_argument, 0, 	'F'},
			{ "to",		required_argument, 0, 	'T'},
		  	{0, 0, 0, 0}
		  };
		int optindex = 0;

		c = getopt_long(argc, argv, "hlu:d:F:T:", long_options, &optindex);
		if (c == -1)
			break;

		switch (c) {
			case 'l':
				list = 1;
				break;
			case 'u':
				username = strdup(optarg);
				break;
			case 'd':
				device = strdup(optarg);
				break;
			case 'F':
				time_from = strdup(optarg);
				break;
			case 'T':
				time_to = strdup(optarg);
				break;
			case 'h':
			case '?':
				/* getopt_long already printed message */
				usage(progname);
				break;
			default:
				abort();
		}

	}

	argc -= (optind);
	argv += (optind);

	if (!username && device) {
		fprintf(stderr, "%s: device name without username doesn't make sense\n", progname);
		return (-2);
	}

	if (make_times(time_from, &s_lo, time_to, &s_hi) != 1) {
		fprintf(stderr, "%s: bad time(s) specified\n", progname);
		return (-2);
	}

	if (list) {
		char *js;

		json = lister(username, device, s_lo, s_hi);
		if (json == NULL) {
			fprintf(stderr, "%s: cannot list\n", progname);
			exit(2);
		}
		js = json_stringify(json, " ");
		printf("%s\n", js);
		json_delete(json);
		free(js);

		return (0);
	}

	if (argc == 0 && !username && !device) {
		fprintf(stderr, "%s: nothing to do. Specify filename or --user and --device\n", progname);
		return (-1);
	} else if (username && device && (argc > 0)) {
		fprintf(stderr, "%s: filename with --user and --device is not supported\n", progname);
		return (-1);
	} else if (!username || !device) {
		usage(progname);
	}


	

	/*
	 * If any files were passed on the command line, we assume these are *.rec
	 * and process those. Otherwise, we expect a `user' and `device' and process
	 * "today"
	 */

	obj = json_mkobject();
	locs = json_mkarray();


	if (argc) {
		int n;

		for (n = 0; n < argc; n++) {
			locations(argv[n], obj, locs);
		}
	} else {
		JsonNode *arr, *f;

		if ((json = lister(username, device, s_lo, s_hi)) != NULL) {
			if ((arr = json_find_member(json, "results")) != NULL) { // get array
				json_foreach(f, arr) {
					printf("%s\n", f->string_);
					locations(f->string_, obj, locs);
				}
			}
			json_delete(json);
		}
	}

	json_append_member(obj, "locations", locs);
	printf("%s\n", json_stringify(obj, " "));

	return (0);
}
