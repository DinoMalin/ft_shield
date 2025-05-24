#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>

#define NAME "trojan"

#define FILES "src/trojan/client.c", "src/trojan/main.c",			\
				"src/trojan/password.c", "src/trojan/server.c",		\
		 		"src/trojan/shell.c"

int main() {
	int out = open("/dev/null", O_WRONLY);
	if (fork() == 0) {
		dup2(1, out);
		dup2(2, out);
		execl("/bin/cc", "/bin/cc", FILES, "-Llib", "-lft", "-o", NAME, "-Iinclude");
		exit(0);
	} else {
		printf("jcario\n");
	}
}
