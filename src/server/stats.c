#include "server.h"

static unsigned long long prev_user = 0, prev_nice = 0, prev_system = 0,
    prev_idle = 0;
static unsigned long long prev_iowait = 0, prev_irq = 0, prev_softirq = 0,
    prev_steal = 0;

void get_sys_stats(char *buffer, size_t size)
{
	FILE *fp = fopen("/proc/stat", "r");
	if (!fp)
		return;

	unsigned long long user, nice, system, idle, iowait, irq, softirq,
	    steal;
	if (fscanf
	    (fp, "cpu  %llu %llu %llu %llu %llu %llu %llu %llu", &user, &nice,
	     &system, &idle, &iowait, &irq, &softirq, &steal) != 8) {
		fclose(fp);
		return;
	}
	fclose(fp);

	unsigned long long prev_idle_total = prev_idle + prev_iowait;
	unsigned long long idle_total = idle + iowait;

	unsigned long long prev_non_idle = prev_user + prev_nice + prev_system +
	    prev_irq + prev_softirq + prev_steal;
	unsigned long long non_idle =
	    user + nice + system + irq + softirq + steal;

	unsigned long long prev_total = prev_idle_total + prev_non_idle;
	unsigned long long total = idle_total + non_idle;

	unsigned long long totald = total - prev_total;
	unsigned long long idled = idle_total - prev_idle_total;

	float cpu_usage = 0.0;
	if (totald != 0) {
		cpu_usage = ((float)(totald - idled) / totald) * 100.0;
	}

	prev_user = user;
	prev_nice = nice;
	prev_system = system;
	prev_idle = idle;
	prev_iowait = iowait;
	prev_irq = irq;
	prev_softirq = softirq;
	prev_steal = steal;

	size_t mem_total = 0, mem_available = 0;
	fp = fopen("/proc/meminfo", "r");
	if (fp) {
		char line[256];
		while (fgets(line, sizeof(line), fp)) {
			if (sscanf(line, "MemTotal: %zu kB", &mem_total) == 1)
				continue;
			if (sscanf(line, "MemAvailable: %zu kB", &mem_available)
			    == 1)
				continue;
		}
		fclose(fp);
	}

	size_t mem_used = (mem_total - mem_available) / 1024;
	size_t mem_total_mb = mem_total / 1024;

	snprintf(buffer, size, "STATS %.1f %zu %zu", cpu_usage, mem_used,
		 mem_total_mb);
}
