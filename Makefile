OS := $(shell uname)
ifeq ($(OS), Linux)
CC = gcc
LD = gcc
LDLIBS =  -pthread
INC_PATH =
endif
ifeq ($(OS), Darwin)
CC = clang
LD = clang
LDLIBS = 
INC_PATH = 
endif
CFLAGS = -g -Wall
LDFLAGS = -pthread
SRCS = src/buffer.c src/channel.c
SRCS_MAIN = src/main.c
HEADERS = src/buffer.h src/channel.h
SRCS_TEST = test/test.c
OBJECTS = bin/buffer.o bin/channel.o
OBJS_TEST = bin/test.o
OBJS_MAIN = bin/main.o
EXEC_TEST = bin/test
EXEC_MAIN = bin/main
test : $(OBJECTS) $(OBJS_TEST) $(EXEC_TEST)
build : $(OBJECTS) $(OBJS_MAIN) $(EXEC_MAIN)

$(EXEC_TEST):	$(OBJECTS)
				$(LD) $(CFLAGS) $(LDFLAGS) -o $(EXEC_TEST) $(OBJECTS) $(OBJS_TEST) $(LDLIBS) $(INC_PATH)

$(EXEC_MAIN):	$(OBJECTS) $(OBJS_LIB)
				$(LD) $(CFLAGS) $(LDFLAGS) -o $(EXEC_MAIN) $(OBJECTS) $(OBJS_LIB) $(OBJS_MAIN) $(LDLIBS) $(INC_PATH)

$(OBJS_LIB):	$(SRCS_LIB) $(HEADERS_LIB) $(HEADERS)
				$(CC) -c $(SRCS_LIB) $(CFLAGS) $(INC_PATH)

$(OBJECTS):	bin/%.o : src/%.c
				$(CC) $(CFLAGS) -c $< $(LIB_PATH) $(LIBS) -o $@

$(OBJS_TEST):	$(SRCS_TEST) $(HEADERS)
				$(CC) -c $(SRCS_TEST) -o $(OBJS_TEST) $(CFLAGS) $(INC_PATH)

$(OBJS_MAIN):	$(SRCS_MAIN) $(HEADERS) $(HEADERS_LIB)
				$(CC) -c $(SRCS_MAIN) -o $(OBJS_MAIN) $(CFLAGS) $(INC_PATH)

clean:
				rm $(EXEC_MAIN) $(OBJECTS) $(OBJS_MAIN)
clean_test:
				rm $(EXEC_TEST) $(OBJECTS) $(OBJS_TEST)
