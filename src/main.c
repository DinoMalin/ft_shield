#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdbool.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/wait.h>
#include "libft.h"

#define DEBUG printf
#define MAX_CLIENTS 3
#define PORT 6970

char *clean_join(char *s1, char *s2) {
	char *tmp = ft_strjoin(s1, s2);
	free(s1);
	return tmp;
}

char *get_file(int fd) {
	char *l = NULL;
	char *res = ft_strdup("");

	do {
		free(l);
		l = get_next_line(fd);
		if (l)
			res = clean_join(res, l);
	} while (l);

	return res;
}

int init_socket(struct sockaddr_in *addr) {
	int sock = socket(PF_INET, SOCK_STREAM, 0);
	if (sock < 0) {
		DEBUG("socket does not work");
		return 0;
	}

	addr->sin_family = AF_INET;
	addr->sin_addr.s_addr = htonl(INADDR_ANY);
	addr->sin_port = htons(PORT);

	if (bind(sock, (struct sockaddr*)addr, sizeof(struct sockaddr)) < 0) {
		DEBUG("bind does not work");
		return 0;
	}

	if (listen(sock, MAX_CLIENTS) < 0) {
		DEBUG("listen does not work");
		return 0;
	}
	return sock;
}

bool new_connection(
	int sock, int epollfd, struct sockaddr_in *addr, struct epoll_event *ev) {
	socklen_t len = sizeof(struct sockaddr);
	int conn = accept(sock, (struct sockaddr*)addr, &len);
	if (conn < 0) {
		DEBUG("accept does not work");
		return false;
	}

	printf("New connection !\n");
	ev->events = EPOLLIN;
	ev->data.fd = conn;
	if (epoll_ctl(epollfd, EPOLL_CTL_ADD, conn, ev) < 0) {
		DEBUG("epoll_ctl does not work");
		return false;
	}

	return true;
}

void disconnect_client(int fd, int epollfd, struct epoll_event *ev) {
	printf("Client disconnected\n");
	if (epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, ev) < 0)
		DEBUG("epoll_ctl does not work");
}

char *readline(int fd) {
	char *cmd = ft_strdup("");
	char buff[BUFFER_SIZE] = {};

	while (1) {
		int ret = recv(fd, buff, BUFFER_SIZE-1, MSG_DONTWAIT);
		if (ret == 0) {
			free(cmd);
			return NULL;
		} else if (ret > 0) {
			buff[ret] = 0;
			cmd = clean_join(cmd, buff);
		} else {
			if (errno == EAGAIN || errno == EWOULDBLOCK) {
				return cmd;
			}
			free(cmd);
			return NULL;
		}
	}

	return cmd;
}

int main() {
	struct sockaddr_in addr = {};
	struct epoll_event ev, events[MAX_CLIENTS];

	int sock = init_socket(&addr);
	if (!sock)
		return 0;

	int epollfd = epoll_create1(0);
	if (epollfd < 0) {
		DEBUG("epoll_create1 does not work");
		return 0;
	}

	ev.events = EPOLLIN;
	ev.data.fd = sock;

	if (epoll_ctl(epollfd, EPOLL_CTL_ADD, sock, &ev)) {
		DEBUG("epoll_ctl does not work");
		return 0;
	}

	while (true) {
		int nfds = epoll_wait(epollfd, events, MAX_CLIENTS, -1);
		if (nfds == -1) {
			DEBUG("epoll_wait does not work");
			return 0;
		}

		for (int n = 0; n < nfds; n++) {
			int fd = events[n].data.fd;
			if (fd == sock) {
				new_connection(sock, epollfd, &addr, &ev);
			} else {
				char *cmd = readline(fd);
				if (!cmd) {
					disconnect_client(fd, epollfd, &ev);
				}

				int pipefd[2];
				if (pipe(pipefd) < 0) {
					DEBUG("pipe does not work");
					return 0;
				}
				int pid = fork();

				if (pid < 0) {
					DEBUG("pipe does not work");
					return 0;
				} else if (!pid) {
					close(pipefd[0]);
					if (dup2(pipefd[1], 1) < 0) { // have to find a way to make the stdin work too
						DEBUG("dup2 does not work");
						exit(0);
					}
					close(pipefd[1]);
					execl("/bin/sh", "sh", "-c", cmd, (char *) NULL);

					free(cmd);
					close(sock);
					exit(0);
				}
				close(pipefd[1]);

				char *res = get_file(pipefd[0]);
				if (res) {
					send(fd, res, ft_strlen(res), 0);
				}

				free(cmd);
				free(res);
			}
		}
	}

	close(sock);
}
