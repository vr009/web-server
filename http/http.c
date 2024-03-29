#include "http.h"
#include "urldecode.h"
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <stdio.h>
#include <sys/errno.h>
#include <sys/stat.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>


enum REQUEST_METHOD {
	GET = 0,
	POST = 1,
	HEAD = 2,
	PUT = 3,
	DELETE = 4,
	UNKNOWN = 10,
};

enum HTTP_VERSION {
	VER_1_0 = 0,
	VER_1_1 = 1,
	VER_2_0 = 2,
};

struct http_request {
	enum REQUEST_METHOD req_method;
	enum HTTP_VERSION version;
	char * url;
	char * host;
	char * body;
	char * buf;
};

struct http_response {
	enum HTTP_VERSION version;
	int code;
	int content_length;
	char * content_type;
	char * server;
	char * date;
	char * connection;
	char * additional_headers;
	char * body;
	char * file_path;
	enum REQUEST_METHOD req_method;

	int fd; // file to send
	int sd; // socket descriptor
};

struct config {
	char * root_path;
	char * file_name;
};

const char * t = "HTTP/1.1 200 OK\r\nDate: Mon, 27 Jul 2009 12:28:53 GMT\r\n"
				 "Server: web\r\nContent-Length: 5000000000\r\nConnection: Closed\r\n";

void http_request_free(http_request * req) {
	if (req->url) free(req->url);
	if (req->host) free(req->host);
	if (req->buf && strcmp(req->buf, "")) free(req->buf);
	if (req->body) free(req->body);
	if (req) free(req);
}

void http_response_free(http_response * resp) {
	if (resp->content_type != NULL) free(resp->content_type);
	if (resp->date)  free(resp->date);
	if (resp->body) free(resp->body);
	if (resp->additional_headers) free(resp->additional_headers);
	if (resp->connection) free(resp->connection);
	if (resp->file_path) free(resp->file_path);
	if (resp->server) free(resp->server);
	if (resp) free(resp);
}


size_t parse_method(char * buf, http_request * req) {
	size_t cursor = 0;
	while (buf[cursor] != ' ' && cursor < strlen("OPTIONS")) {
		cursor++;
	}

	if (cursor > strlen("OPTIONS"))
		return 0;

	if (strncmp(buf, "GET", strlen("GET")) == 0)
		req->req_method = GET;
	else if (strncmp(buf, "POST", strlen("POST")) == 0)
		req->req_method = POST;
	else if (strncmp(buf, "PUT", strlen("PUT")) == 0)
		req->req_method = PUT;
	else if (strncmp(buf, "HEAD", strlen("HEAD")) == 0)
		req->req_method = HEAD;
	else if (strncmp(buf, "DELETE", strlen("DELETE")) == 0)
		req->req_method = DELETE;
	else
		req->req_method = UNKNOWN;
	return cursor;
}

size_t parse_url(char * buf, size_t pos, http_request * req) {
	size_t url_sz = pos+1;
	while (buf[url_sz] != ' ' && buf[url_sz] != 'H' && buf[url_sz] != '\n' && buf[url_sz] != '\?') {
		url_sz++;
	}
	if (req->url != NULL) {
		free(req->url);
	}
	if (buf[url_sz-1] == '/') {
		req->url = calloc(url_sz - pos + strlen("index.html")+1, sizeof(char));
		strncpy(req->url, buf+pos+1, url_sz-pos-1);
		strcat(req->url, "index.html");
	} else {
		req->url = calloc(url_sz-pos+1, sizeof(char));
		strncpy(req->url, buf+pos+1, url_sz-pos-1);
	}
	return url_sz;
}

size_t parse_version(char * buf, size_t pos, http_request * req) {
	while (buf[pos] != 'H') {
		pos++;
	}
	while (buf[pos] != '/') {
		pos++;
	}
	size_t pos_end = pos;
	while (buf[pos_end] != '\r') {
		pos_end++;
	}

	if ((buf[pos + 1] == '1') && (buf[pos_end - 1] == '1')) {
		req->version = VER_1_1;
	} else if ((buf[pos + 1] == '1') && (buf[pos_end - 1] == '0')) {
		req->version = VER_1_0;
	}
	return pos + strlen("HTTP/1.1\r\n");
}

size_t parse_host(char * buf, size_t pos, http_request * req) {
	while (pos + 3 < strlen(buf) && !(buf[pos] == 'H' && buf[pos+1] == 'o' && buf[pos+2] == 's' && buf[pos+3] == 't')) {
		pos++;
	}
	pos += strlen("Host: ");
	size_t pos_end = pos;
	while (pos_end < strlen(buf) && buf[pos_end] != '\r') {
		pos_end++;
	}
	if (req->host != NULL) {
		free(req->host);
	}
	req->host = calloc(pos_end-pos+1, sizeof(char));
	strncpy(req->host, buf+pos, pos_end - pos);
	return pos_end + strlen("\r\n");
}

void perform_url(http_request* req);

void parse_request(char * buf, http_request * req) {
	if (strlen(buf) < strlen("get / http/1.0\r\n"))
		return;

	// first, parsing method
	size_t cursor = 0;

	cursor = parse_method(buf, req);

	// parsing url
	cursor = parse_url(buf, cursor, req);
	perform_url(req);

	// parsing version
	cursor = parse_version(buf, cursor, req);

	// parsing host
	cursor = parse_host(buf, cursor, req);

	// the rest goes to buf :)
	if (req->buf != NULL) {
		free(req->buf);
	}
	req->buf = buf;
}

void define_content_type(http_request * req, http_response * resp) {
	size_t pos = strlen(req->url);
	while(req->url[pos] != '.') {
		pos--;
	}
	if (req->url[pos+1] == 'h' && req->url[pos+2] == 't' && req->url[pos+3] == 'm') {
		resp->content_type = calloc(strlen("text/html") + 1, sizeof(char));
		resp->content_type = strcpy(resp->content_type, "text/html");
		return;
	}
	if (req->url[pos+1] == 'c' && req->url[pos+2] == 's' && req->url[pos+3] == 's') {
		resp->content_type = calloc(strlen("text/css") + 1, sizeof(char));
		resp->content_type = strcpy(resp->content_type, "text/css");
		return;
	}
	if (req->url[pos+1] == 'j' && req->url[pos+2] == 's') {
		resp->content_type = calloc(strlen("application/javascript") + 1, sizeof(char));
		resp->content_type = strcpy(resp->content_type, "application/javascript");
		return;
	}
	if (req->url[pos+1] == 'j' && req->url[pos+2] == 'p' && req->url[pos+3] == 'g') {
		resp->content_type = calloc(strlen("image/jpeg") + 1, sizeof(char));
		resp->content_type = strcpy(resp->content_type, "image/jpeg");
		return;
	}
	if (req->url[pos+1] == 'j' && req->url[pos+2] == 'p' && req->url[pos+3] == 'e') {
		resp->content_type = calloc(strlen("image/jpeg") + 1, sizeof(char));
		resp->content_type = strcpy(resp->content_type, "image/jpeg");
		return;
	}
	if (req->url[pos+1] == 'p' && req->url[pos+2] == 'n' && req->url[pos+3] == 'g') {
		resp->content_type = calloc(strlen("image/png") + 1, sizeof(char));
		resp->content_type = strcpy(resp->content_type, "image/png");
		return;
	}
	if (req->url[pos+1] == 'g' && req->url[pos+2] == 'i' && req->url[pos+3] == 'f') {
		resp->content_type = calloc(strlen("image/gif") + 1, sizeof(char));
		resp->content_type = strcpy(resp->content_type, "image/gif");
		return;
	}
	if (req->url[pos+1] == 's' && req->url[pos+2] == 'w' && req->url[pos+3] == 'f') {
		resp->content_type = calloc(strlen("application/x-shockwave-flash") + 1, sizeof(char));
		resp->content_type = strcpy(resp->content_type, "application/x-shockwave-flash");
		return;
	}
}

void define_date(http_response * resp) {
	long int s_time;
	struct tm m_time;

	s_time = time (NULL);
	char buf[26] = "";
	localtime_r (&s_time, &m_time);

	resp->date = calloc(sizeof("Mon, 27 Jul 2009 12:28:53 GMT"), sizeof(char));
	sprintf(resp->date, "%s GMT", asctime_r(&m_time, buf));
}

void send_all(int sock_d, char * msg) {
	size_t left = strlen(msg);
	ssize_t sent = 0;
	while (sent < left) {
		long snt = send(sock_d, msg + sent, strlen(msg) - sent, 0);
		if (snt == -1) {
			snt = snt;
			exit(1);
		}
		sent += snt;
	}
}

void send_headers(int sock_d, http_response * resp) {
	size_t msg_len = strlen(t);
	define_date(resp);
	char * buf = calloc(msg_len*2, sizeof(char));
	char * template = "%s %d %s\r\nDate: %s\r\nServer: web\r\nContent-Length: %d\r\nContent-Type: %s\r\nConnection: %s\r\n\r\n";

	char version[9] = "HTTP/1.0";
	char answer_msg[19] = "OK";

	if (resp->version == VER_1_1) {
		version[7] = '1';
	}
	if (resp->code == 404) {
		strcpy(answer_msg, "NOT FOUND");
	} else if (resp->code == 403) {
		strcpy(answer_msg, "FORBIDDEN");
	} else if (resp->code == 405) {
		strcpy(answer_msg, "METHOD NOT ALLOWED");
	}
	sprintf(buf, template, version, resp->code, answer_msg, resp->date, resp->content_length, resp->content_type, "Closed");
	send_all(sock_d, buf);
	free(buf);
}

void perform_url(http_request* req) {
	req->url = urlDecode(req->url);
}

int url_is_bad(char * url) {
	if (strstr(url, "../") == NULL) {
		return 0;
	}
	return 1;
}

void prepare_response(int sock_d, http_request* req, http_response * resp, struct config * cfg) {
	resp->sd = sock_d;
	resp->req_method = req->req_method;
	if (req->req_method != GET && req->req_method != HEAD) {
		resp->code = 405;
		resp->content_length = 0;
		return;
	}
	if (url_is_bad(req->url)) {
		resp->code = 404;
		resp->content_length = 0;
		return;
	}
	char * file_abs_path = calloc(strlen(req->url) + strlen(cfg->root_path) + 1, sizeof(char));
	file_abs_path = strcat(file_abs_path, cfg->root_path);
	file_abs_path = strcat(file_abs_path, req->url);

	FILE * f = fopen(file_abs_path, "r+");
	if (f == NULL) {
		if (errno == EACCES) {
			resp->code = 403;
		} else if (errno == ENOENT && strcmp(file_abs_path + strlen(file_abs_path) - strlen("index.html"), "index.html") == 0) {
			resp->code = 403;
		} else {
			resp->code = 404;
		}
		resp->content_length = 0;
	} else {
		resp->code = 200;
		struct stat statistics;
		int fd = fileno(f);
		resp->fd = fd;
		if (fstat(fd, &statistics) != -1) {
			resp->content_length = statistics.st_size;
		}
		define_content_type(req, resp);
	}
	free(file_abs_path);
}

void send_response(struct http_response * resp) {
	send_headers(resp->sd, resp);
	if (resp->code == 200 && resp->req_method != HEAD) {
		int fd = resp->fd;
		int sd = resp->sd;

		long long file_bytes_sent = resp->content_length;
		long long sent = 0;

		int res = -1;
		while(1) {
#if defined(__linux__)
			res = sendfile(sd, fd, 0, resp->content_length);
#elif defined(__APPLE__)
			res = sendfile(fd, sd, 0, &file_bytes_sent, NULL,  0);
#endif
			sent += file_bytes_sent;
			while (sent < resp->content_length) {
				res = sendfile(fd, sd, sent, &file_bytes_sent, NULL,  0);
				sent += file_bytes_sent;
			}
			if (res == 0) {
				break;
			}
			if (res == -1 && errno != EAGAIN) {
				break;
			}
		}
		close(resp->fd);
	} else if (resp->code == 200 && resp->req_method == HEAD) {
		close(resp->fd);
	}
}

void http_cb(int sd, char * root_path) {
	struct config cfg;
	if (root_path == NULL) {
		cfg.root_path = calloc(sizeof("/var/www/html"), sizeof(char));
		cfg.root_path = strcpy(cfg.root_path, "/var/www/html");
	} else {
		cfg.root_path = root_path;
	}

	cfg.file_name = "index.html";

	struct http_request * req = (http_request*)malloc(sizeof(http_request));
	req->url = req->host = req->buf = req->body = NULL;

	struct http_response * resp = (http_response*) malloc(sizeof(http_response));
	resp->content_type = resp->date = resp->body = resp->server = resp->file_path =
			resp->connection = resp->additional_headers = NULL;

	char * buf = calloc(1000, sizeof(char));
	char * tmp_buf = calloc(800, sizeof(char));
	int rcvd = 0;
	while(buf[rcvd] != '\n') {
		int rvd = recv(sd, tmp_buf, sizeof(tmp_buf) - 1, MSG_DONTWAIT);
		if (rvd == 0)
			break;
		if (rvd != -1) {
			rcvd += rvd;
			buf = strcat(buf, tmp_buf);
			memset(tmp_buf, 0, strlen(tmp_buf));
		}
		if (rvd == -1 && errno != EAGAIN) {
			free(buf);
			free(tmp_buf);
			http_request_free(req);
			http_response_free(resp);
			return;
		} else if (rvd == -1) {
			break;
		}
	}
	parse_request(buf, req);
	if (req->url == NULL || strcmp(req->url, "") == 0) {
		free(tmp_buf);
		http_request_free(req);
		http_response_free(resp);
		return;
	}
	prepare_response(sd, req, resp, &cfg);
	send_response(resp);

	free(tmp_buf);
	http_request_free(req);
	http_response_free(resp);
}
