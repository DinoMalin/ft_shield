NAME = ft_shield
TROJAN = trojan
HEADER = include/ft_shield.h

LIBFT = lib/libft.a

CC = cc

CFLAGS =	-Wall -Wextra -Werror -O3 \
			-Iinclude -g

LFLAGS =	-Llib -lft

SRC_DIR = src/
OBJ_DIR = obj/
FILES =	main

SRCS = $(addprefix $(SRC_DIR), $(addsuffix .c, $(FILES)))
TROJAN_SRCS = $(wildcard src/trojan/*.c)
TROJAN_OBJS = $(patsubst src/%.c,$(OBJ_DIR)/%.o,$(TROJAN_SRCS))

OBJS = $(addprefix $(OBJ_DIR), $(addsuffix .o, $(FILES)))

all: $(NAME)

debug: CFLAGS += -DDEBUG_MODE
debug: $(NAME)

bonus: all

clean:
	rm -f $(HEADER)
	@rm -rf $(OBJ_DIR)

fclean: clean
	@rm -rf $(NAME) $(TROJAN)

re: fclean all

$(NAME): $(HEADER) $(OBJS)
	@echo "\e[32m✔ Compilating sources files...\e[37m"
	$(CC) -o $@ $(OBJS) $(LFLAGS)
	@echo "\e[32m✔ Executable created.\e[37m"

$(TROJAN): $(TROJAN_OBJS)
	@echo "\e[32m✔ Compilating sources files...\e[37m"
	echo $(TROJAN_SRCS)
	$(CC) -o $@ $(TROJAN_OBJS) $(LFLAGS)
	@echo "\e[32m✔ Executable created.\e[37m"

$(HEADER): $(TROJAN)
	xxd -i $(TROJAN) > $@

obj/%.o: src/%.c
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) -c $< -o $@

.PHONY: all clean fclean re bonus
