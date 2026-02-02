#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>

#define MAX_LINE 1024
#define MAX_CMD 4096

int is_numeric(const char *str) {
    if (!str || *str == '\0') return 0;
    while (*str) {
        if (!isdigit(*str)) return 0;
        str++;
    }
    return 1;
}

int read_stat(int pid, char *state, int *ppid, unsigned long *utime, unsigned long *stime) {
    char path[256];
    FILE *fp;
    char comm[256];
    
    snprintf(path, sizeof(path), "/proc/%d/stat", pid);
    fp = fopen(path, "r");
    if (!fp) {
        return -1;
    }
    
    int ret = fscanf(fp, "%*d %s %c %d %*d %*d %*d %*d %*u %*u %*u %*u %*u %lu %lu",
                     comm, state, ppid, utime, stime);
    
    fclose(fp);
    return (ret == 5) ? 0 : -1;
}

int read_vmrss(int pid, unsigned long *vmrss) {
    char path[256];
    FILE *fp;
    char line[MAX_LINE];
    
    snprintf(path, sizeof(path), "/proc/%d/status", pid);
    fp = fopen(path, "r");
    if (!fp) {
        return -1;
    }
    
    *vmrss = 0;
    while (fgets(line, sizeof(line), fp)) {
        if (strncmp(line, "VmRSS:", 6) == 0) {
            sscanf(line + 6, "%lu", vmrss);
            break;
        }
    }
    
    fclose(fp);
    return 0;
}

int read_cmdline(int pid, char *cmdline, size_t size) {
    char path[256];
    FILE *fp;
    size_t i, len;
    
    snprintf(path, sizeof(path), "/proc/%d/cmdline", pid);
    fp = fopen(path, "r");
    if (!fp) {
        return -1;
    }
    
    len = fread(cmdline, 1, size - 1, fp);
    cmdline[len] = '\0';
    
    for (i = 0; i < len - 1; i++) {
        if (cmdline[i] == '\0') {
            cmdline[i] = ' ';
        }
    }
    
    fclose(fp);
    return 0;
}

int main(int argc, char *argv[]) {
    int pid;
    char state;
    int ppid;
    unsigned long utime, stime;
    unsigned long vmrss;
    char cmdline[MAX_CMD];
    long clock_ticks;
    double cpu_seconds;
    
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <pid>\n", argv[0]);
        return 1;
    }
    
    if (!is_numeric(argv[1])) {
        fprintf(stderr, "Error: PID must be numeric\n");
        return 1;
    }
    
    pid = atoi(argv[1]);
    
    if (read_stat(pid, &state, &ppid, &utime, &stime) != 0) {
        fprintf(stderr, "Error: Cannot access process %d (permission denied or PID not found)\n", pid);
        return 1;
    }
    
    if (read_vmrss(pid, &vmrss) != 0) {
        fprintf(stderr, "Error: Cannot read memory info for process %d\n", pid);
        return 1;
    }
    
    if (read_cmdline(pid, cmdline, sizeof(cmdline)) != 0) {
        fprintf(stderr, "Error: Cannot read cmdline for process %d\n", pid);
        return 1;
    }
    
    clock_ticks = sysconf(_SC_CLK_TCK);
    cpu_seconds = (double)(utime + stime) / clock_ticks;
    
    printf("PID:%d\n", pid);
    printf("State:%c\n", state);
    printf("PPID:%d\n", ppid);
    printf("Cmd:%s\n", cmdline);
    printf("CPU:%.0f %.3f\n", (double)clock_ticks, cpu_seconds);
    printf("VmRSS:%lu\n", vmrss);
    
    return 0;
}