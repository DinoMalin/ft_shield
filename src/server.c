#include "ft_shield.h"

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

int new_connection(
	int sock, int epollfd, struct sockaddr_in *addr, struct epoll_event *ev) {
	socklen_t len = sizeof(struct sockaddr);
	int fd = accept(sock, (struct sockaddr*)addr, &len);
	if (fd < 0) {
		DEBUG("accept does not work");
		return -1;
	}

	ev->events = EPOLLIN|EPOLLRDHUP;
	ev->data.fd = fd;
	if (epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, ev) < 0) {
		DEBUG("epoll_ctl does not work");
		return -1;
	}

	DEBUG("New connection !\n");
	return fd;
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
