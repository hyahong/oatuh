GCC			= gcc
GCCFLAGS	= #-Wall -Wextra -Werror

MAIN		= oatuh.c parser.c

SRCS		= $(addprefix srcs/, $(MAIN))
INCS		= incs/
LIBS		= -lssl -lcrypto

OBJS		= $(SRCS:.c=.o)

NAME		= liboatuh.a

%.o: %.c
	@echo "\033[32m"compiling file '$<'"\033[0m"
	@$(GCC) -c -o $@ $< $(GCCFLAGS) -I$(INCS) $(LIBS) 

all		:	$(NAME)

$(NAME)	:	$(OBJS)
			@ar rcs $(NAME) $(OBJS)
			@echo "\033[32m"successfully linking library ">" $(NAME)"\033[0m"

clean	:
			@echo "\033[31m"remove object files $(OBJS)"\033[0m"
			@rm -f $(OBJS)

fclean	:	clean
			@echo "\033[31m"remove output $(NAME)"\033[0m"
			@rm -f $(NAME)

re		:	fclean all

test	:	re
			@echo "\033[32m"compile with main.c ..."\033[0m"
			@gcc main.c -L./ -loatuh -lssl -lcrypto
			@echo "==========================="
			@./a.out
			@echo "\n==========================="
			@rm a.out

.PHONY	:	all clean fclean re