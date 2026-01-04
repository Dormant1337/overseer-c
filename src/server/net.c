#include "server.h"

int form_message(void)
{
	server_id = rand() % 9000 + 1000;
	snprintf(beacon_msg, sizeof(beacon_msg), "0.0.0.0 %d %d", tcp_port,
		 server_id);
	return 0;
}

int setup_server(int port)
{
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0)
		return -1;

	int opt = 1;
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(port);

	if (bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		log_msg(KRED, "Error: Could not bind to port %d", port);
		return -1;
	}
	if (listen(sockfd, 5) < 0) {
		log_msg(KRED, "Error: Could not listen");
		return -1;
	}
	return sockfd;
}

void *send_beacon_thread(void *arg)
{
	int sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock < 0)
		pthread_exit(NULL);

	int broadcastEnable = 1;
	setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcastEnable,
		   sizeof(broadcastEnable));

	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(BEACON_PORT);
	addr.sin_addr.s_addr = inet_addr("255.255.255.255");

	log_msg(KMAG, "Beacon Broadcasting on UDP port %d", BEACON_PORT);

	while (running) {
		sendto(sock, beacon_msg, strlen(beacon_msg), 0,
		       (struct sockaddr *)&addr, sizeof(addr));
		sleep(2);
	}
	close(sock);
	pthread_exit(NULL);
}
