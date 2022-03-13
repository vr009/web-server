#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <strings.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "ev.h"

/* client number limitation */
#define MAX_CLIENTS 1000

/* message length limitation */
#define MAX_MESSAGE_LEN (256)

#define err_message(msg) \
    do {perror(msg); exit(EXIT_FAILURE);} while(0)

/* record the number of clients */
static int client_number;

static int create_serverfd(char const *addr, uint16_t u16port)
{
	int fd;
	struct sockaddr_in server;

	fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd < 0) err_message("socket err\n");

	server.sin_family = AF_INET;
	server.sin_port = htons(u16port);
	inet_pton(AF_INET, addr, &server.sin_addr);

	if (bind(fd, (struct sockaddr *)&server, sizeof(server)) < 0) err_message("bind err\n");

	if (listen(fd, 10) < 0) err_message("listen err\n");

	return fd;
}

static void read_cb(EV_P_ ev_io *watcher, int revents)
{
	ssize_t ret;
	char buf[MAX_MESSAGE_LEN] = {0};

	ret = recv(watcher->fd, buf, sizeof(buf) - 1, MSG_DONTWAIT);
	if (ret > 0) {
		write(watcher->fd, buf, ret);

	} else if ((ret < 0) && (errno == EAGAIN || errno == EWOULDBLOCK)) {
		return;

	} else {
		fprintf(stdout, "client closed (fd=%d)\n", watcher->fd);
		--client_number;
		ev_io_stop(EV_A_ watcher);
		close(watcher->fd);
		free(watcher);
	}
}

static void accept_cb(EV_P_ ev_io *watcher, int revents)
{
	int connfd;
	ev_io *client;

	connfd = accept(watcher->fd, NULL, NULL);
	if (connfd > 0) {
		if (++client_number > MAX_CLIENTS) {
			close(watcher->fd);

		} else {
			client = calloc(1, sizeof(*client));
			ev_io_init(client, read_cb, connfd, EV_READ);
			ev_io_start(EV_A_ client);
		}

	} else if ((connfd < 0) && (errno == EAGAIN || errno == EWOULDBLOCK)) {
		return;

	} else {
		close(watcher->fd);
		ev_break(EV_A_ EVBREAK_ALL);
		/* this will lead main to exit, no need to free watchers of clients */
	}
}

static void start_server(char const *addr, uint16_t u16port)
{
	int fd;
#ifdef EV_MULTIPLICITY
	struct ev_loop *loop;
#else
	int loop;
#endif
	ev_io *watcher;

	fd = create_serverfd(addr, u16port);
//	loop = ev_default_loop(EVFLAG_NOENV);
	loop = ev_loop_new(EVFLAG_NOENV);
	watcher = calloc(1, sizeof(*watcher));
	printf("here: %p , %p", loop, watcher);
	assert(("can not alloc memory\n", loop && watcher));

	/* set nonblock flag */
	fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK);

	ev_io_init(watcher, accept_cb, fd, EV_READ);
	ev_io_start(EV_A_ watcher);
	ev_run(EV_A_ 0);

	ev_loop_destroy(EV_A);
	free(watcher);
}

static void signal_handler(int signo)
{
	switch (signo) {
		case SIGPIPE:
			break;
		default:
			// unreachable
			break;
	}
}

int * workers_pids;

int fork_workers(int cpu_count, int * workers_pids) {
	for (int i = 0; i < cpu_count; i++) {
		int pid = fork();
		if (pid < 0) {
			return -1;
		} else if (pid == 0) {
			continue;
		} else {
			workers_pids[i] = pid;
			break;
		}
	}
	return 0;
}

int main() {
	int port = 8084;

//	int ncpus ;
//	ncpus = sysconf(_SC_NPROCESSORS_CONF);
//	printf("cpus: %d\n", ncpus);
//	workers_pids = (int*)calloc(ncpus, sizeof(int));
//	int res = fork_workers(ncpus, workers_pids);
//	if (res == -1) {
//		return res;
//	}

	signal(SIGPIPE, signal_handler);
	start_server("127.0.0.1", port);

	return 0;
}

void helper() {
	//	int sd = socket(PF_INET, SOCK_STREAM, 0);
//	if (sd == -1) {
//		return -1;
//	}
//
//	struct sockaddr_in serv_addr;
//	serv_addr.sin_family = AF_INET;
//	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
//	serv_addr.sin_port = htons(port);
//	int n = bind(sd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
//	if (n == -1) {
//		return -1;
//	}
//
//	listen(sd, 100);
}
