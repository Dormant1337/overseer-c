#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <signal.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <errno.h>

#define BEACON_PORT 9999
#define BEACON_MSG_SIZE 256

#define KNRM  "\x1B[0m"
#define KRED  "\x1B[31m"
#define KGRN  "\x1B[32m"
#define KYEL  "\x1B[33m"
#define KBLU  "\x1B[34m"
#define KMAG  "\x1B[35m"
#define KCYN  "\x1B[36m"
#define KWHT  "\x1B[37m"

int server_id = 0;
int tcp_port = 8080;
char *server_password = "admin";
char beacon_msg[BEACON_MSG_SIZE];
volatile bool running = true;
int server_socket_fd = -1;

unsigned long long prev_user = 0, prev_nice = 0, prev_system = 0, prev_idle = 0;
unsigned long long prev_iowait = 0, prev_irq = 0, prev_softirq = 0, prev_steal = 0;

void log_msg(const char *color, const char *format, ...)
{
	time_t rawtime;
	struct tm *timeinfo;
	char buffer[80];

	time(&rawtime);
	timeinfo = localtime(&rawtime);
	strftime(buffer, sizeof(buffer), "%H:%M:%S", timeinfo);

	printf("%s[%s] ", KNRM, buffer);
	printf("%s", color);

	va_list args;
	va_start(args, format);
	vprintf(format, args);
	va_end(args);

	printf("%s\n", KNRM);
}

void handle_signal(int sig)
{
	printf("\n");
	log_msg(KRED, "Received shutdown signal...");
	running = false;
	if (server_socket_fd != -1) {
		close(server_socket_fd);
	}
}

int form_message(void)
{
	server_id = rand() % 9000 + 1000;
	snprintf(beacon_msg, sizeof(beacon_msg), "0.0.0.0 %d %d", tcp_port, server_id);
	return 0;
}

int setup_server(int port)
{
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) return -1;

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
	if (sock < 0) pthread_exit(NULL);

	int broadcastEnable = 1;
	setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcastEnable, sizeof(broadcastEnable));

	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(BEACON_PORT);
	addr.sin_addr.s_addr = inet_addr("255.255.255.255");

	log_msg(KMAG, "Beacon Broadcasting on UDP port %d", BEACON_PORT);

	while (running) {
		sendto(sock, beacon_msg, strlen(beacon_msg), 0, (struct sockaddr *)&addr, sizeof(addr));
		sleep(2);
	}
	close(sock);
	pthread_exit(NULL);
}

void get_sys_stats(char *buffer, size_t size)
{
	FILE *fp = fopen("/proc/stat", "r");
	if (!fp) return;

	unsigned long long user, nice, system, idle, iowait, irq, softirq, steal;
	if (fscanf(fp, "cpu  %llu %llu %llu %llu %llu %llu %llu %llu",
		   &user, &nice, &system, &idle, &iowait, &irq, &softirq, &steal) != 8) {
		fclose(fp);
		return;
	}
	fclose(fp);

	unsigned long long prev_idle_total = prev_idle + prev_iowait;
	unsigned long long idle_total = idle + iowait;

	unsigned long long prev_non_idle = prev_user + prev_nice + prev_system + prev_irq + prev_softirq + prev_steal;
	unsigned long long non_idle = user + nice + system + irq + softirq + steal;

	unsigned long long prev_total = prev_idle_total + prev_non_idle;
	unsigned long long total = idle_total + non_idle;

	unsigned long long totald = total - prev_total;
	unsigned long long idled = idle_total - prev_idle_total;

	float cpu_usage = 0.0;
	if (totald != 0) {
		cpu_usage = ((float)(totald - idled) / totald) * 100.0;
	}

	prev_user = user; prev_nice = nice; prev_system = system; prev_idle = idle;
	prev_iowait = iowait; prev_irq = irq; prev_softirq = softirq; prev_steal = steal;

	size_t mem_total = 0, mem_available = 0;
	fp = fopen("/proc/meminfo", "r");
	if (fp) {
		char line[256];
		while (fgets(line, sizeof(line), fp)) {
			if (sscanf(line, "MemTotal: %zu kB", &mem_total) == 1) continue;
			if (sscanf(line, "MemAvailable: %zu kB", &mem_available) == 1) continue;
		}
		fclose(fp);
	}

	size_t mem_used = (mem_total - mem_available) / 1024;
	size_t mem_total_mb = mem_total / 1024;

	snprintf(buffer, size, "STATS %.1f %zu %zu", cpu_usage, mem_used, mem_total_mb);
}

void handle_file_transfer(int client_fd, const char *header_info)
{
	char filename[256];
	size_t filesize;
	sscanf(header_info, "FILE %s %zu", filename, &filesize);

	log_msg(KCYN, "Receiving File: %s (%zu bytes)", filename, filesize);

	struct stat st = {0};
	if (stat("storage", &st) == -1) {
		mkdir("storage", 0700);
	}

	char filepath[512];
	snprintf(filepath, sizeof(filepath), "storage/%s", filename);

	FILE *fp = fopen(filepath, "wb");
	if (!fp) {
		log_msg(KRED, "Error opening file for write");
		return;
	}

	send(client_fd, "GO", 2, 0);

	char buffer[8192];
	size_t received = 0;
	while (received < filesize) {
		ssize_t n = recv(client_fd, buffer, sizeof(buffer), 0);
		if (n <= 0) break;
		fwrite(buffer, 1, n, fp);
		received += n;
	}
	
	fclose(fp);
	log_msg(KGRN, "File Saved: %s", filepath);
}

void handle_execution(int client_fd, const char *command_line)
{
	const char *cmd = command_line + 5;
	log_msg(KYEL, "Executing: %s", cmd);

	FILE *fp = popen(cmd, "r");
	if (fp == NULL) {
		const char *err = "Error: Failed to execute command.\n";
		send(client_fd, err, strlen(err), 0);
		return;
	}

	char path[1024];
	while (fgets(path, sizeof(path), fp) != NULL) {
		send(client_fd, path, strlen(path), 0);
	}

	pclose(fp);
	log_msg(KGRN, "Execution complete");
}

bool authenticate_connection(int client_fd, struct sockaddr_in client_addr)
{
	char buf[256];
	char *client_ip = inet_ntoa(client_addr.sin_addr);

	ssize_t n = recv(client_fd, buf, sizeof(buf) - 1, 0);
	if (n <= 0) return false;
	
	buf[n] = '\0';

	if (strncmp(buf, "AUTH ", 5) == 0) {
		char *pass = buf + 5;
		if (strcmp(pass, server_password) == 0) {
			send(client_fd, "OK", 2, 0);
			return true;
		}
	}

	log_msg(KRED, "Auth Failed from %s", client_ip);
	send(client_fd, "ERR", 3, 0);
	return false;
}

void handle_client(int client_fd, struct sockaddr_in client_addr)
{
	if (!authenticate_connection(client_fd, client_addr)) {
		close(client_fd);
		return;
	}

	char buf[1024];
	char *client_ip = inet_ntoa(client_addr.sin_addr);

	ssize_t n = recv(client_fd, buf, sizeof(buf) - 1, 0);
	if (n > 0) {
		buf[n] = '\0';
		
		if (strncmp(buf, "FILE", 4) == 0) {
			handle_file_transfer(client_fd, buf);
		} else if (strncmp(buf, "EXEC", 4) == 0) {
			handle_execution(client_fd, buf);
		} else if (strncmp(buf, "STATS", 5) == 0) {
			char stats_buf[128];
			get_sys_stats(stats_buf, sizeof(stats_buf));
			send(client_fd, stats_buf, strlen(stats_buf), 0);
		} else {
			log_msg(KCYN, "CMD from %s: %s", client_ip, buf);
			const char *response = "ACK: Command Received";
			send(client_fd, response, strlen(response), 0);
		}
	}
	close(client_fd);
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

	log_msg(KGRN, "TCP Server Listening on port %d (ID: %d)", tcp_port, server_id);
	log_msg(KBLU, "Password protected: %s", server_password);

	while (running) {
		struct sockaddr_in client_addr;
		socklen_t len = sizeof(client_addr);

		int client_fd = accept(server_socket_fd, (struct sockaddr *)&client_addr, &len);
		if (client_fd >= 0) {
			handle_client(client_fd, client_addr);
		}
	}

	log_msg(KYEL, "System Shutdown Complete.");
	close(server_socket_fd);
	pthread_join(beacon_thread, NULL);
	return 0;
}