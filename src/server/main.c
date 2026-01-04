#include "server.h"

int server_id = 0;
int tcp_port = 8080;
char *server_password = "admin";
char beacon_msg[BEACON_MSG_SIZE];
volatile bool running = true;
int server_socket_fd = -1;

void handle_signal(int sig)
{
	printf("\n");
	log_msg(KRED, "Received shutdown signal...");
	running = false;
	if (server_socket_fd != -1) {
		close(server_socket_fd);
	}
}

int main(int argc, char *argv[])
{
	srand((unsigned int)time(NULL));
	signal(SIGINT, handle_signal);

	if (argc > 1) {
		tcp_port = atoi(argv[1]);
	}
	if (argc > 2) {
		server_password = argv[2];
	}

	log_msg(KWHT, "--- SYSTEM BOOT ---");
	form_message();

	pthread_t beacon_thread;
	if (pthread_create(&beacon_thread, NULL, send_beacon_thread, NULL) != 0) {
		return 1;
	}

	server_socket_fd = setup_server(tcp_port);
	if (server_socket_fd < 0) {
		running = false;
		pthread_join(beacon_thread, NULL);
		return 1;
	}

	log_msg(KGRN, "TCP Server Listening on port %d (ID: %d)", tcp_port,
		server_id);
	log_msg(KBLU, "Password protected: %s", server_password);

	while (running) {
		struct sockaddr_in client_addr;
		socklen_t len = sizeof(client_addr);

		int client_fd =
		    accept(server_socket_fd, (struct sockaddr *)&client_addr,
			   &len);
		if (client_fd >= 0) {
			handle_client(client_fd, client_addr);
		}
	}

	log_msg(KYEL, "System Shutdown Complete.");
	close(server_socket_fd);
	pthread_join(beacon_thread, NULL);
	return 0;
}
