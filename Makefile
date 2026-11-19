# Name
NAME		= ircserv

# Directories

OBJ_DIR		= obj
CLASS_DIR	= classes
INCL_DIR	= includes

# Sources
CLASS_SRC		= $(CLASS_DIR)/Channel.cpp \
					$(CLASS_DIR)/Client.cpp \
					$(CLASS_DIR)/Parser.cpp \
					$(CLASS_DIR)/Server.cpp	\
					$(CLASS_DIR)/Utils.cpp
SOURCES			= main.cpp \
					$(CLASS_SRC)

# Objects
OBJECTS			= $(SOURCES:%.cpp=$(OBJ_DIR)/%.o)
DEP				= $(OBJECTS:.o=.d)

# Compiler
CC			= c++
CFLAGS		= -Wall -Wextra -Werror -Wshadow
CEXTRA_VER	= -std=c++98
CEXTRA_INC	= -I$(INCL_DIR) -I. -MMD -MP
CPP_FLAGS	= $(CFLAGS) $(CEXTRA_VER) $(CEXTRA_INC)

# Rules
all: $(OBJ_DIR) $(NAME)

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(NAME): $(OBJECTS)
	$(CC) $(CPP_FLAGS) $(OBJECTS) -o $@

$(OBJ_DIR)/%.o: %.cpp
	mkdir -p $(dir $@)
	$(CC) $(CPP_FLAGS) -c $< -o $@

run: all
	./ircserv 6667 pass

run_valgrind: all
	valgrind --leak-check=full --show-leak-kinds=all --track-fds=all \
		--errors-for-leak-kinds=definite \
	./ircserv 6667 pass

clean:
	rm -rf $(OBJ_DIR)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all run run_valgrind clean fclean re

-include $(DEP)
-include test/stress_test.mk
