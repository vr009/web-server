#ifndef WEB_SERVER_HTTP_H
#define WEB_SERVER_HTTP_H

typedef struct http_request http_request;
typedef struct http_response http_response;

typedef struct config config;

void send_response(int sock_d, http_request* req, http_response * resp, struct config * cfg);

void parse_request(char * buf, http_request * req);

//void recv_http_request(int sock_d, char * buf);

void request_init(http_request * req);

void response_init(http_response * resp);

void http_request_free(http_request * req);

void http_response_free(http_response * resp);

void test_cb(int sd, char * root_path);

#endif //WEB_SERVER_HTTP_H
