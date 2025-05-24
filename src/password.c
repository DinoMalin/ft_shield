#include "ft_shield.h"

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

void check_password(Client *client, char *line) {
	if (fnv1a(line) == HASHED_PASSWORD) {
		client->logged = true;
	} else {
		PUTSTR(client->fd, "Password: ");
	}
}
