#include "ft_shield.h"

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

void disconnect_client(Client *client, int epollfd, struct epoll_event *ev) {
	DEBUG("Client disconnected !\n");
	if (epoll_ctl(epollfd, EPOLL_CTL_DEL, client->fd, ev) < 0)
		DEBUG("epoll_ctl does not work");
	close(client->fd);

	client->fd = -1;
	client->logged = false;
}
