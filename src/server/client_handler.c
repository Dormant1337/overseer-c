#include "server.h"

void handle_file_transfer(int client_fd, const char *header_info)
{
	char filename[256];
	size_t filesize;
	sscanf(header_info, "FILE %s %zu", filename, &filesize);

	log_msg(KCYN, "Receiving File: %s (%zu bytes)", filename, filesize);

	struct stat st = { 0 };
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
		if (n <= 0)
			break;
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
	if (n <= 0)
		return false;

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
