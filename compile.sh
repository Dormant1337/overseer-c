#!/bin/bash

rm -f client server

echo "Compiling Server..."
gcc src/server/main.c \
	src/server/utils.c \
	src/server/stats.c \
	src/server/net.c \
	src/server/client_handler.c \
	-o server -lpthread

if [ $? -eq 0 ]; then
	echo "Server compiled successfully."
else
	echo "Server compilation failed!"
	exit 1
fi

echo "Compiling Client..."
gcc src/client/main.c \
	src/client/system/network.c \
	src/client/system/api.c \
	src/client/system/atomic.c \
	src/client/tui/render.c \
	src/client/tui/components.c \
	src/client/tui/popups.c \
	src/client/tui/popup_file.c \
	src/client/tui/input.c \
	src/client/tui/path_security.c \
	-o client \
	-lncurses -lpthread -latomic

if [ $? -eq 0 ]; then
	echo "Client compiled successfully."
else
	echo "Client compilation failed!"
	exit 1
fi

echo "Done. Run ./server (optionally with port) and ./client"