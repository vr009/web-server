#include "config_parser.h"
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>


size_t parse_cpu(char * buf, struct spec_config * sp_cfg) {
	size_t pos = 0;
	while (pos + 2 < strlen(buf) && !(buf[pos] == 'c' && buf[pos+1] == 'p' && buf[pos+2] == 'u')) {
		pos++;
	}
	if (pos + 2 == strlen(buf)) {
		return 0;
	}
	while (buf[pos] != ' ') {
		pos++;
	}
	while (buf[pos] != '1' && buf[pos] != '2' && buf[pos] != '3' && buf[pos] != '4' &&
		buf[pos] != '5' && buf[pos] != '6'&& buf[pos] != '7' && buf[pos] != '8' && buf[pos] != '9') {
		pos++;
	}
	char * num = calloc(2, sizeof(char));
	num = strncpy(num, buf + pos, 1);

	sp_cfg->cpus = atoi(num);

	return pos;
}

size_t parse_root(char * buf, struct spec_config * sp_cfg) {
	size_t pos = 0;
	while (pos + 2 < strlen(buf) && !(buf[pos] == 'd' && buf[pos+1] == 'o' && buf[pos+2] == 'c')) {
		pos++;
	}
	while (buf[pos] != ' ') {
		pos++;
	}
	size_t end_pos = pos;
	while (end_pos + 1 < strlen(buf) && buf[end_pos + 1] != ' ' && buf[end_pos + 1] != '\n') {
		end_pos++;
	}
	sp_cfg->root = calloc(end_pos-pos+1, sizeof(char));
	strncpy(sp_cfg->root, buf+pos+1, end_pos-pos);
	return end_pos;
}

size_t parse_script_path(char * buf, struct spec_config * sp_cfg) {
	char * start = strstr(buf, "script");
	if (start == NULL) {
		return 0;
	}
	if (start + strlen("script") <= buf + strlen(buf)) {
		char * pos1 = start + strlen("script") + 1;
		start += strlen("script") + 1;
		while (strcmp(start, " ") == 0) {
			start++;
		}
		pos1 = start;
		while (strcmp(start, " ") != 0 && start + 1 != buf + strlen(buf) + 1) {
			start++;
		}
		char * pos2 = start;
		size_t name_size = pos2 - pos1;
		char * res = calloc(name_size, sizeof(char));
		strncpy(res, pos1, name_size);
		sp_cfg->script_path = res;
		return 1;
	}
	return 0;
}

struct spec_config * parse_spec(char * spec_path, struct spec_config * sp_cfg){
	FILE * f = fopen(spec_path, "r+");
	if (f == NULL) {
		return NULL;
	}
	size_t length = 0;

	struct stat statistics;
	int fd = fileno(f);

	if (fstat(fd, &statistics) != -1) {
		length = statistics.st_size;
	}

	char * buf = calloc(length, sizeof(char));
	if (fread(buf , sizeof(char), length, f) != length) {
		fclose(f);
		return NULL;
	}

	size_t res = parse_cpu(buf, sp_cfg);
	if (res == 0) {
		fclose(f);
		return sp_cfg;
	}
	res = parse_root(buf, sp_cfg);
	if (res == 0) {
		fclose(f);
		return sp_cfg;
	}

	res = parse_script_path(buf, sp_cfg);
	if (res == 0) {
		fclose(f);
		return sp_cfg;
	}

	fclose(f);
	return sp_cfg;
}
