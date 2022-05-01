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
#include <pthread.h>

#include "ev.h"
#include "http.h"
#include "config_parser.h"

int * pids;

static int tasks_running;
static int limit;

static struct spec_config * cfg;

//static int * workers_statistic;

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
	server.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(fd, (struct sockaddr *)&server, sizeof(server)) < 0) err_message("bind err\n");

	if (listen(fd, 1000000) < 0) err_message("listen err\n");

	return fd;
}

static void idle_cb(struct ev_loop *loop, ev_idle *watcher, int revents)
{
	sleep(0);
}

struct thread_data {
	void * data;
	struct ev_loop *read_loop;
	struct ev_loop *write_loop;
};

static void read_cb(EV_P_ ev_io *watcher, int revents)
{
	struct thread_data * th_data = (struct thread_data *)watcher->data;
	struct ev_loop * read_loop = th_data->read_loop;

	http_response * resp;
	resp = http_cb(watcher->fd, cfg->root);
	send_response(resp);

	close(watcher->fd);
	ev_io_stop(read_loop, watcher);

	free(resp);
	free(watcher);
}

int clear_zombies() {
	int cleared = 0;
	for (int i = 0; i < tasks_running; i++) {
		int status = 0;
		int res = waitpid(pids[i], &status, WNOHANG);
		if (res > 0 || WIFEXITED(status)) {
			cleared++;
			pids[i] = 0;
		}
	}
	return cleared;
}


static void accept_cb(EV_P_ ev_io *watcher, int revents)
{
	struct thread_data * th_data = (struct thread_data *)watcher->data;
	struct ev_loop *read_loop = th_data->read_loop;

	int connfd;
	ev_io *client;
	connfd = accept(watcher->fd, NULL, NULL);

	if (connfd > 0) {
		client = calloc(1, sizeof(*client));
		client->data = th_data;
		ev_io_init(client, read_cb, connfd, EV_READ);
		ev_io_start(read_loop, client);
//		fprintf(stdout, "accept_cb from %d\n", getpid());
	} else if ((connfd < 0) && (errno == EAGAIN || errno == EWOULDBLOCK)) {
		return;

	} else {
		close(watcher->fd);
		ev_break(EV_A_ EVBREAK_ALL);
		/* this will lead main to exit, no need to free watchers of clients */
	}
}

void * read_thread(void * args) {
	struct ev_loop *read_loop = (struct ev_loop *)args;

	ev_idle *idle_watcher;
	idle_watcher = (ev_idle *)calloc(1, sizeof(*idle_watcher));
	ev_idle_init(idle_watcher, idle_cb);
	fprintf(stdout, "read_loop in thread %p\n", read_loop);
	ev_idle_start(read_loop, idle_watcher);

	fprintf(stdout, "forked to read thread at pid=%d\n", getpid());

	ev_run(read_loop, 0);
	fprintf(stdout, "returning\n");
	return 0;
}


static void start_server(char const *addr, uint16_t u16port)
{
	int fd;
#ifdef EV_MULTIPLICITY
	struct ev_loop *loop; // == accept_loop
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

	for (int i = 0; i < limit; i++) {
		int pid = fork();
		if (pid == -1) {
			return;
		}
		if (pid == 0) {
			struct ev_loop *read_loop;

			struct thread_data * th_data = malloc(sizeof(struct thread_data));

			read_loop = ev_loop_new(EVBACKEND_KQUEUE);

			th_data->read_loop = read_loop;

			pthread_t reader_thread;

			pthread_create(&reader_thread, NULL, read_thread, read_loop);

			watcher = calloc(1, sizeof(*watcher));
			assert(("can not alloc memory\n", loop && watcher));
			watcher->data = th_data;

			/* set nonblock flag */
			fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK);
			ev_io_init(watcher, accept_cb, fd, EV_READ);
			ev_io_start(EV_A_ watcher);

			ev_loop_fork(loop);
			ev_run(EV_A_ 0);
			fprintf(stdout, "DESTROYED\n");
			tasks_running--;
			ev_loop_destroy(EV_A);
			free(watcher);
			pthread_join(reader_thread, NULL);
			fprintf(stdout, "destroyed\n");
			ev_loop_destroy(read_loop);
		} else {
			pids[i] = pid;
			continue;
		}
	}
	while (tasks_running != 0) {
		sleep(10);
	}
	write(STDOUT_FILENO, "CLEARING ZOMBIES", strlen("CLEARING ZOMBIES"));
	clear_zombies();

	ev_loop_destroy(EV_A);
	free(watcher);
}

static void signal_handler(int signo)
{
	switch (signo) {
		case SIGPIPE:
			break;
		case SIGINT:
		{
			write(STDOUT_FILENO, "\n", strlen("\n"));
			char *buf = calloc(sizeof("pid: 1234567\n count of works: 1234567\n"), sizeof(char));
			for (int i = 0; i < limit; i++) {
				write(STDOUT_FILENO, buf, strlen(buf));
			}
			free(buf);
		}
			break;
		case SIGCHLD:
			wait(NULL);
			break;
		default:
			// unreachable
			break;
	}
}


int main(int argc, char *argv[]) {
	int port = 82;

	tasks_running = 8;

	cfg = malloc(sizeof(struct spec_config));

	int max_possible_cpus = sysconf(_SC_NPROCESSORS_CONF);

	if (argc > 1) {
		parse_spec(argv[1], cfg);
	} else {
		parse_spec("/etc/httpd.conf", cfg);
	}
	limit = cfg->cpus < max_possible_cpus ? cfg->cpus : max_possible_cpus;
	if (limit == 0) {
		limit = max_possible_cpus;
	}
	limit = limit * 2;

	pids = (int*)mmap(NULL, sizeof(long) * limit , PROT_READ | PROT_WRITE,
	                  MAP_SHARED | MAP_ANONYMOUS, -1, 0);

	if (cfg->root == NULL) {
		cfg->root = calloc(sizeof("/var/www/html"), sizeof(char));
		strncpy(cfg->root, "/var/www/html", strlen("/var/www/html"));
	}
	write(STDOUT_FILENO, cfg->root, strlen(cfg->root));
	write(STDOUT_FILENO, "\n", strlen("\n"));

	signal(SIGPIPE, signal_handler);
	signal(SIGINT, signal_handler);
	signal(SIGCHLD, SIG_IGN);

	write(STDOUT_FILENO, "STARTING SERVER", strlen("STARTING SERVER"));
	start_server("0.0.0.0", port);

	free(cfg);
	return 0;
}
