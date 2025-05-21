#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdbool.h>
#include <fcntl.h>
#include <errno.h>
#include "libft.h"

#define DEBUG printf
#define MAX_CLIENTS 3
#define PORT 6970

char *clean_join(char *s1, char *s2) {
	char *tmp = ft_strjoin(s1, s2);
	free(s1);
	return tmp;
}

int init(struct sockaddr_in *addr) {
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

int main() {
	struct sockaddr_in addr = {};
	struct epoll_event ev, events[MAX_CLIENTS];
	socklen_t len = sizeof(struct sockaddr);
	char *cmd = ft_strdup("");

	int sock = init(&addr);
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
			if (events[n].data.fd == sock) { // new client
				int conn = accept(sock, (struct sockaddr*) &addr, &len);
				if (conn < 0) {
					DEBUG("accept does not work");
					return 0;
				}

				printf("New connection !\n");
				ev.events = EPOLLIN;
				ev.data.fd = conn;
				if (epoll_ctl(epollfd, EPOLL_CTL_ADD, conn, &ev) < 0) {
					DEBUG("epoll_ctl does not work");
					return 0;
				}
			} else {
				char buff[BUFFER_SIZE] = {};

				while (1) {
					int ret = recv(events[n].data.fd, buff, BUFFER_SIZE-1, MSG_DONTWAIT);
					if (ret < 0) {
						if (errno == EAGAIN || errno == EWOULDBLOCK) {
							printf("cmd: [%s]\n", cmd);
						}
						free(cmd);
						cmd = ft_strdup("");
						break;
					} else if (ret == 0) {
						printf("Client disconnected\n");
						if (epoll_ctl(epollfd, EPOLL_CTL_DEL, events[n].data.fd, &ev) < 0) {
							DEBUG("epoll_ctl does not work");
							return 0;
						}
						break;
					} else {
						buff[ret] = 0;
						cmd = clean_join(cmd, buff);
					}
				}
			}
		}
	}

	close(sock);
}
