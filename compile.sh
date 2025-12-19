#!/bin/bash

# deliting old executables
rm -f client server

echo "Compiling Server..."
gcc src/server/main.c -o server -lpthread

if [ $? -eq 0 ]; then
    echo "Server compiled successfully."
else
    echo "Server compilation failed!"
    exit 1
fi

echo "Compiling Client..."
#starting compiling client...
gcc src/client/main.c \
    src/client/system/network.c \
    src/client/tui/interface.c \
    -o client \
    -lncurses -lpthread -latomic

if [ $? -eq 0 ]; then
    echo "Client compiled successfully."
else
    echo "Client compilation failed!"
    exit 1
fi

echo "Done. Run ./server (optionally with port) and ./client"
