#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/resource.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <vector>

#define HZ 100

#define min(a,b) (a<b?a:b)
#define max(a,b) (a>b?a:b)

struct process_t {
    pid_t pid;
    bool is_limited;
};

std::vector<process_t> process_list;

float limit;
pid_t limit_pid = -1;

struct process_screenshot {
	struct timespec when;
	int jiffies;
	int cputime;
};

struct cpu_usage {
	float pcpu;
	float workingrate;
};

void quit(int sig)
{
	//printf("Exiting...\n");
    if (limit_pid != -1) {
        //printf("resume...\n");
        kill(limit_pid, SIGCONT);
    }

    exit(0);
}

void getpidof(const char *process)
{
    std::vector<process_t> new_process_list;

	if (setpriority(PRIO_PROCESS, getpid(), 19) != 0) {
		printf("Warning: cannot renice\n");
	}

	char exelink[20];
	char exepath[PATH_MAX + 1];

    DIR *dip;
    struct dirent *dit;

    if ((dip = opendir("/proc")) == NULL) {
        perror("opendir");
        return;
    }

    while ((dit = readdir(dip)) != NULL) {
        pid_t pid = atoi(dit->d_name);
        if (pid > 0) {
            sprintf(exelink, "/proc/%d/exe", pid);
            int size = readlink(exelink, exepath, sizeof(exepath));
            if (size > 0) {
                int found = 0;
                if (process[0] == '/' && strncmp(exepath, process, size) == 0 && size == strlen(process)) {
                    found = 1;
                } else {
                    if (strncmp(exepath + size - strlen(process), process, strlen(process)) == 0) {
                        found = 1;
                    }
                }
                if (found == 1) {
                    if (kill(pid, SIGSTOP) == 0 && kill(pid, SIGCONT) == 0) {
                        process_t process;
                        process.pid = pid;
                        process.is_limited = false;
                        new_process_list.push_back(process);
                    } else {
                        fprintf(stderr,"Error: Process %d detected, but you don't have permission to control it\n", pid);
                    }
                }
            }
        }
    }

    if (closedir(dip) == -1) {
        perror("closedir");
        return;
    }

    for (uint32_t i = 0; i < new_process_list.size(); ++i) {
        for (uint32_t j = 0; j < process_list.size(); ++j) {
            if (new_process_list[i].pid == process_list[i].pid) {
                new_process_list[i].is_limited = process_list[i].is_limited;
                break;
            }
        }
    }

    process_list = new_process_list;
}

inline long timediff(const struct timespec *ta,const struct timespec *tb)
{
    unsigned long us = (ta->tv_sec - tb->tv_sec) * 1000000 + (ta->tv_nsec / 1000 - tb->tv_nsec / 1000);
    return us;
}

int getjiffies(int pid)
{
	static char stat[20];
	static char buffer[1024];
	sprintf(stat, "/proc/%d/stat", pid);
	FILE *f = fopen(stat,"r");
	if (f == NULL)
        return -1;

	fgets(buffer, sizeof(buffer), f);
	fclose(f);

	char *p = buffer;
	p = (char *)memchr((const void *)(p + 1), ')', sizeof(buffer) - (p - buffer));
	int sp = 12;
	while (sp--)
		p = (char *)memchr((const void *)(p + 1), ' ', sizeof(buffer) - (p - buffer));

	int utime = atoi(p + 1);
	p = (char *)memchr((const void *)(p + 1), ' ', sizeof(buffer) - (p - buffer));

	int ktime = atoi(p + 1);
	return utime + ktime;
}

int compute_cpu_usage(int pid, int last_working_quantum, struct cpu_usage *pusage)
{
	#define MEM_ORDER 10
	static struct process_screenshot ps[MEM_ORDER];
	static int front = -1;
	static int tail = 0;

	if (pusage == NULL) {
		front = -1;
		tail = 0;
		return 0;
	}

	front = (front + 1) % MEM_ORDER;
	int j = getjiffies(pid);
	if (j >= 0)
        ps[front].jiffies = j;
	else
        return -1;

	clock_gettime(CLOCK_REALTIME, &(ps[front].when));
	ps[front].cputime = last_working_quantum;

	int size = (front - tail + MEM_ORDER) % MEM_ORDER + 1;

	if (size == 1) {
		pusage->pcpu = -1;
		pusage->workingrate = 1;
		return 0;
	} else {
		long dt = timediff(&(ps[front].when), &(ps[tail].when));
		long dtwork = 0;
		int i = (tail + 1) % MEM_ORDER;
		int max = (front + 1) % MEM_ORDER;
		do {
			dtwork += ps[i].cputime;
			i = (i + 1) % MEM_ORDER;
		} while (i != max);

		int used = ps[front].jiffies-ps[tail].jiffies;
		float usage = (used * 1000000.0 / HZ) / dtwork;
		pusage->workingrate = 1.0 * dtwork / dt;
		pusage->pcpu = usage * pusage->workingrate;
		if (size == MEM_ORDER)
			tail = (tail + 1) % MEM_ORDER;

		return 0;
	}
	#undef MEM_ORDER
}

void cpulimit(pid_t pid)
{
    limit_pid = pid;

	compute_cpu_usage(0,0,NULL);

	int i = 0;

	int period = 100000;
	struct timespec twork, tsleep;
	memset(&twork, 0, sizeof(struct timespec));
	memset(&tsleep, 0, sizeof(struct timespec));
	struct timespec startwork, endwork;
	long workingtime = 0;
	float pcpu_avg = 0;

	while(1) {
		struct cpu_usage cu;
		if (compute_cpu_usage(pid, workingtime, &cu) == -1) {
			exit(0);
		}

		float pcpu = cu.pcpu;
		float workingrate = cu.workingrate;

		if (pcpu > 0) {
			twork.tv_nsec = min(period * limit * 1000 / pcpu * workingrate, period * 1000);
		} else if (pcpu == 0) {
			twork.tv_nsec = period * 1000;
		} else if (pcpu == -1) {
			pcpu = limit;
			workingrate = limit;
			twork.tv_nsec = min(period * limit * 1000, period * 1000);
		}

		tsleep.tv_nsec = period * 1000 - twork.tv_nsec;

		pcpu_avg = (pcpu_avg * i + pcpu) / (i + 1);

		if (limit < 1 && limit > 0) {
			if (kill(pid, SIGCONT) != 0) {
                exit(0);
			}
		}

		clock_gettime(CLOCK_REALTIME, &startwork);
		nanosleep(&twork, NULL);
		clock_gettime(CLOCK_REALTIME, &endwork);
		workingtime = timediff(&endwork, &startwork);

		if (limit < 1) {
			if (kill(pid, SIGSTOP)!=0) {
                exit(0);
			}
			nanosleep(&tsleep, NULL);
		}
		i++;
	}
}

int main(int argc, char **argv)
{
    if (argc != 3) {
        printf("usage: cpulimit name limit\n");
        exit(0);
    }

    daemon(1, 1);

	const char *exe = argv[1];
	int perclimit = atoi(argv[2]);

	limit = perclimit / 100.0;
	if (limit < 0 || limit > 1) {
		fprintf(stderr,"Error: limit must be in the range 0-100\n");
		exit(1);
	}

	signal(SIGINT,quit);
	signal(SIGTERM,quit);

    while (1) {
        sleep(2);
        getpidof(exe);
        for (uint32_t i = 0; i < process_list.size(); ++i) {
            if (!process_list[i].is_limited) {
                process_list[i].is_limited = true;
                pid_t pid = fork();
                if (pid < 0) {
                    continue;
                } else if (pid > 0) {
                    while (true) {
                         int status = 0;
                         if (-1 == waitpid(-1, &status, 0))
                            continue;
                         else
                            break;
                    }
                } else {
                    pid_t pid = fork();
                    if (pid < 0) {
                        exit(0);
                    } else if (pid > 0) {
                        exit(0);
                    } else {
                        cpulimit(process_list[i].pid);
                        exit(0);
                    }
                }
            }
        }
    }
}
