#include "server.h"

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
