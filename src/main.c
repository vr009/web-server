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
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "ev.h"
#include "../http/http.h"
#include "../config_parser/config_parser.h"

/* client number limitation */
#define MAX_CLIENTS 10000

int * pids;

static int tasks_running;
static int limit;

static struct spec_config * cfg;

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
//	inet_pton(AF_INET, addr, &server.sin_addr);
	server.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(fd, (struct sockaddr *)&server, sizeof(server)) < 0) err_message("bind err\n");

	if (listen(fd, 10000) < 0) err_message("listen err\n");

	return fd;
}

static void read_cb(EV_P_ ev_io *watcher, int revents)
{
	test_cb(watcher->fd, cfg->root);
	ev_io_stop(EV_A_ watcher);
	close(watcher->fd);
	free(watcher);
}

int clear_zombies() {
	int cleared = 0;
	for (int i = 0; i < tasks_running; i++) {
		int status = 0;
		int res = waitpid(pids[i], &status, WNOHANG);
		if (res > 0 || WIFEXITED(status)) {
//			write(STDOUT_FILENO, "CLEARED A ZOMBIE", strlen("CLEARED A ZOMBIE"));
			cleared++;
			pids[i] = 0;
		}
	}
	return cleared;
}

static void read_cb2(EV_P_ ev_io *watcher, int revents)
{
	while (tasks_running >= limit) {
//		sleep(1);
		int cleared = clear_zombies();
		if (cleared)
			tasks_running -= cleared;
	}
	int pid = fork();
	if (pid == -1) {
//		write(STDOUT_FILENO, "FAIL", strlen("FAIL"));
		return;
	}
	if (pid == 0) {
//		write(STDOUT_FILENO, "INSIDE WORKER\n", strlen("INSIDE WORKER\n"));
		test_cb(watcher->fd, cfg->root);
		ev_io_stop(EV_A_ watcher);
		close(watcher->fd);
		free(watcher);
//		write(STDOUT_FILENO, "WORKER ENDED UP", strlen("WORKER ENDED UP\n"));
		exit(0);
	} else {
		ev_io_stop(EV_A_ watcher);
		close(watcher->fd);
		free(watcher);
		pids[tasks_running] = pid;
		tasks_running++;
	}
}

static void accept_cb(EV_P_ ev_io *watcher, int revents)
{
	int connfd;
	ev_io *client;

	connfd = accept(watcher->fd, NULL, NULL);
	if (connfd > 0) {
		client = calloc(1, sizeof(*client));
		ev_io_init(client, read_cb2, connfd, EV_READ);
		ev_io_start(EV_A_ client);

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
#if defined(__linux__)
	loop = ev_default_loop(EVBACKEND_EPOLL);
#elif defined(__APPLE__)
	loop = ev_default_loop(EVBACKEND_KQUEUE);
#endif
	watcher = calloc(1, sizeof(*watcher));
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
//	write(STDOUT_FILENO, "IN HANDLER\n", strlen("IN HANDLER\n"));
	switch (signo) {
		case SIGPIPE:
			break;
		case SIGINT:
//			write(STDOUT_FILENO, "SIGINT", strlen("SIGINT"));
			break;
		case SIGCHLD:
//			write(STDOUT_FILENO, "SIGCHLD", strlen("SIGCHLD"));
			wait(NULL);
			break;
		default:
			// unreachable
			break;
	}
}


int main(int argc, char *argv[]) {
	int port = 8084;

	tasks_running = 0;

	cfg = malloc(sizeof(struct spec_config));
//	int ncpus ;
	limit = sysconf(_SC_NPROCESSORS_CONF);
	pids = calloc(limit, sizeof(int));
//	printf("cpus: %d\n", ncpus);
	if (argc > 1) {
		parse_spec(argv[1], cfg);
	} else {
		parse_spec("/etc/httpd.conf", cfg);
	}

	signal(SIGPIPE, signal_handler);
	signal(SIGINT, signal_handler);
	signal(SIGCHLD, SIG_IGN);
//	signal(SIGCHLD, signal_handler);

	start_server("0.0.0.0", port);
	free(cfg);
	return 0;
}
