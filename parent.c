#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>

#define CYCLE_ITERATIONS_COUNT 505
#define MAX_PROCESSES 10
#define TIMER_NANOSECONDS 10000000

typedef struct {
    volatile sig_atomic_t a;
    volatile sig_atomic_t b;
} Data;

Data data = { 0, 0 };
int stat[4] = { 0 };

volatile sig_atomic_t is_working = 1;

struct sigaction allow_signal, disallow_signal, stat_request_signal;

timer_t timerID;

pid_t allProcesses[MAX_PROCESSES];
int process_count = 0;

int is_stdout_open = 1;



void print_statistic(pid_t ppid, pid_t pid) {
    if (is_stdout_open) {
        printf("PPID: %d, PID: %d, 00: %d, 01: %d, 10: %d, 11: %d\n",
            ppid, pid, stat[0], stat[1], stat[2], stat[3]);
    }
}


// CHILD SIGNAL HANDLERS

void sigusr1_handler(int signal) {
    (void)signal;
    is_stdout_open = 1;
}

void sigusr2_handler(int signal)  {
    (void)signal; 
    is_stdout_open = 0;
}

// REG SIGNALS

void signal_handlers() {
    allow_signal.sa_handler = sigusr1_handler;
    allow_signal.sa_flags = SA_RESTART;

    disallow_signal.sa_handler = sigusr2_handler;
    disallow_signal.sa_flags = SA_RESTART;

    sigaction(SIGUSR1, &allow_signal, NULL);
    sigaction(SIGUSR2, &disallow_signal, NULL);
}



// TIMER CONFIGURATION

void alarm_handler(int sig, siginfo_t *si, void *uc) {
    (void)sig; 
    (void)si; 
    (void)uc;

    const int index = data.a * 2 + data.b * 1; 
    stat[index] += 1;

    is_working = 0;
}

void setup_timer() {
    struct sigevent sev;
    struct itimerspec its;
    struct sigaction sa;

    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = alarm_handler;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGRTMIN, &sa, NULL);

    sev.sigev_notify = SIGEV_SIGNAL;
    sev.sigev_signo = SIGRTMIN;
    sev.sigev_value.sival_ptr = &timerID;
    timer_create(CLOCK_REALTIME, &sev, &timerID);

    its.it_value.tv_sec = 0;
    its.it_value.tv_nsec = TIMER_NANOSECONDS;
    its.it_interval.tv_sec = 0;
    its.it_interval.tv_nsec = TIMER_NANOSECONDS;
    timer_settime(timerID, 0, &its, NULL);
}



// CHILD PROCCESSING

void child_process() {
    int counter = 0;
    signal_handlers(); 
    while(1) {
        is_working = 1;
        setup_timer();

        while (is_working) {
            data.a ^= 1;
            data.b ^= 1;
        }
        
        timer_delete(timerID);
        counter++;
        if(counter >= CYCLE_ITERATIONS_COUNT){
            print_statistic(getppid(), getpid());
            counter = 0;
        }
    }
}



// CREATING PROCCESS

void create_child() {
    if (process_count < MAX_PROCESSES) {
        pid_t pid = fork();

        if (pid == -1) {
            perror("Error when creating new process");
            exit(1);
        } if (pid == 0) {
            child_process();
        } if (pid > 0) {
            printf("Parent: Created new process with PID %d\n", pid);
        }

        allProcesses[process_count++] = pid;
    } else {
        printf("Parent: processes limit is reached");
    }
}

// KILLING PROCESSES

void kill_process_by_ind(int ind) {
    if (process_count > 0 && ind < process_count) {
        pid_t pid = allProcesses[ind];

        kill(pid, SIGKILL);

        for (int i = ind + 1; i < process_count; i++) {
            allProcesses[i - 1] = allProcesses[i];
        }

        process_count--;

        printf("Parent: Killed process with PID %d. Remaining: %d\n", pid, process_count);
    } else {
        printf("Parent: No child processes to kill\n");
    }
}

void kill_all_processes() {
    while (process_count > 0) {
        kill_process_by_ind(process_count - 1);
    }

    printf("Parent: Killed all child processes\n");
}



// SHOWING PROCESSES

void show_all_processes() {
    printf("Parent PID: %d\n", getpid());

    for (int i = 0; i < process_count; i++) {
        printf("|---Child PID: %d\n", allProcesses[i]);
    }
}

// INPUT CHECK AND EXTRACTING NUMBER

int letter_num_option_check(char letter, char str[], int len) {
    if (len >= 2 && str[0] == letter) {
        for (int i = 1; i < len; i++) {
            if (str[i] < 48 || str[i] > 57 || i > 3) return 0;
        }
        return 1;
    }
    return 0;
}

int get_num_from_option(char str[], int len) {
    int num = 0;

    for (int i = 1; i < len; i++) {
        if (str[i] < 47 || str[i] > 57) return -1;

        num = num * 10 + ((int)str[i]) - 48;
    }

    return num;
}

// ENABLING TIMER

void setup_enable_timer(timer_t timerid) {
    struct sigevent sev;
    struct sigaction sa;
    struct itimerspec its;

    sa.sa_flags = SA_SIGINFO;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGRTMIN, &sa, NULL);
    
    sev.sigev_notify = SIGEV_SIGNAL;
    sev.sigev_signo = SIGRTMIN;
    sev.sigev_value.sival_ptr = &timerid;
    timer_create(CLOCK_REALTIME, &sev, &timerid);

    its.it_value.tv_sec = 2;
    its.it_value.tv_nsec = 0;
    its.it_interval.tv_sec = 0;
    its.it_interval.tv_nsec = 0;
    timer_settime(timerid, 0, &its, NULL);
}



int main() {
    printf("\nEnter option:\n+ - create child\n- - remove last child\nl - show processes list\nk - kill all childs\nq - exit):\n");
    timer_t timerid;
    int is_should_allow_all = 0;

    while (1) {
        char option[10];
        int len = 0;

        if (scanf("%9s", option) != 1) {
            continue;
        }

        for (int i = 0; option[i] != '\0'; i++) {
            len++;
        }

        if (strcmp(option, "+") == 0) {
            create_child();
        } else if (strcmp(option, "-") == 0) {
            kill_process_by_ind(process_count - 1);
        } else if (strcmp(option, "l") == 0) {
            show_all_processes();
        } else if (strcmp(option, "k") == 0) {
            kill_all_processes();
        }  else if (strcmp(option, "q") == 0) {
            kill_all_processes();
            printf("Parent: Exiting\n");
            break;
        } 
    }

    return 0;
}
