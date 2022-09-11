#include <yaml.h>

#include "yaml_config_parser.h"
#include "config.h"

int is_key(char * str);

struct spec_config * parse_spec(char * spec_path, struct spec_config * sp_cfg) {
	fprintf(stdout, "YAML READING\n");
	yaml_parser_t parser;
	yaml_event_t event;

	int done = 0;

/* Create the Parser object. */
	yaml_parser_initialize(&parser);

/* Set a file input. */
	FILE *input = fopen(spec_path, "rb");

	yaml_parser_set_input_file(&parser, input);

/* Read the event sequence. */
	char * last_value = NULL;
	do {
		if (!yaml_parser_parse(&parser, &event)) {
			printf("Parser error %d\n", parser.error);
			exit(EXIT_FAILURE);
		}

		switch(event.type)
		{
			case YAML_NO_EVENT: puts("No event!"); break;
				/* Stream start/end */
			case YAML_STREAM_START_EVENT: puts("STREAM START"); break;
			case YAML_STREAM_END_EVENT:   puts("STREAM END");   break;
				/* Block delimeters */
			case YAML_DOCUMENT_START_EVENT: puts("<b>Start Document</b>"); break;
			case YAML_DOCUMENT_END_EVENT:   puts("<b>End Document</b>");   break;
			case YAML_SEQUENCE_START_EVENT: puts("<b>Start Sequence</b>"); break;
			case YAML_SEQUENCE_END_EVENT:   puts("<b>End Sequence</b>");   break;
			case YAML_MAPPING_START_EVENT:  puts("<b>Start Mapping</b>");  break;
			case YAML_MAPPING_END_EVENT:    puts("<b>End Mapping</b>");    break;
				/* Data */
			case YAML_ALIAS_EVENT:  printf("Got alias (anchor %s)\n", event.data.alias.anchor); break;
			case YAML_SCALAR_EVENT: {
				printf("Got scalar (value %s)\n", event.data.scalar.value);
				printf("last value %s\n", last_value);
				if (is_key(last_value)) {
					printf("last value is key%s\n", last_value);
					if (strcmp(last_value, "document_root") == 0) {
						sp_cfg->root = calloc(sizeof(event.data.scalar.value), sizeof(char));
						strcpy(sp_cfg->root, event.data.scalar.value);
						fprintf(stdout, "%s", sp_cfg->root);
					} else if (strcmp(last_value, "cpu_limit") == 0) {
						sp_cfg->cpus = atoi(event.data.scalar.value);
					} else if (strcmp(last_value, "script") == 0) {
						sp_cfg->script_path = calloc(sizeof(event.data.scalar.value), sizeof(char));
						strcpy(sp_cfg->script_path, event.data.scalar.value);
						fprintf(stdout, "%s", sp_cfg->script_path);
					} else {
						printf("Got unknown scalar (value %s)\n", event.data.scalar.value);
					}
				}

				if (last_value != NULL) free(last_value);
				last_value = calloc(sizeof(event.data.scalar.value), sizeof(char));
				strcpy(last_value, event.data.scalar.value);

				break;
			}
		}
		if(event.type != YAML_STREAM_END_EVENT)
			yaml_event_delete(&event);
	} while(event.type != YAML_STREAM_END_EVENT);
	yaml_event_delete(&event);

/* Destroy the Parser object. */
	yaml_parser_delete(&parser);

	return sp_cfg;

/* On error. */
	error:

/* Destroy the Parser object. */
	yaml_parser_delete(&parser);

	return NULL;
}

int is_key(char * str) {
	if (str == NULL) {
		return 0;
	}
	return strcmp(str, "cpu_limit") == 0 |
	            strcmp(str, "document_root") == 0 |
	            strcmp(str, "script") == 0;
}
