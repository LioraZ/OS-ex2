#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <wait.h>
#include <string.h>

#define MAX_JOB_LEN 1024
#define MAX_ARGS 20
#define UNSUCCESSFUL_FORK "Unsuccessful fork\n"
#define BAD_ALLOC "Bad memory allocation\n"
#define SYS_CALL_ERR "Error calling system call\n"

typedef struct Job {
    pid_t pid;
    char *jobName;
    char *args[MAX_ARGS];
    struct Job *next;
} Job;

typedef struct {
    Job *first;
    Job *last;
    Job *secondToLast;
    int size;
} JobsQueue;

Job *newJob(char *n_jobName, char *n_args[]);
void cpyArgs(Job *job, char *n_args[]);
void freeArgs(char *args[]);
void deleteJob(Job *job);
JobsQueue *createJobsQueue();
JobsQueue *createJobQueue(Job *job);
void updateLastIndex(JobsQueue *jobsQueue);
int isEmpty(JobsQueue *jobsQueue);
JobsQueue *addJob(JobsQueue *jobsQueue, Job *job);
void removeFirst(JobsQueue *jobsQueue);
void removeJob(JobsQueue *jobsQueue, pid_t pid);
void freeJobsQueue(JobsQueue *jobsQueue);


Job *getPromptJob(int *wait);
void printError(char *error);
void checkForWait(int wait, pid_t pid);

void exitPrompt(char *error);

int main() {
    int wait_;
    JobsQueue *jobsQueue = createJobsQueue();
    if (!jobsQueue) exitPrompt(BAD_ALLOC);
    do {
        Job *job = getPromptJob(&wait_);
        if (!job) break;
        pid_t pid = fork(); //will only get legal job
        if (pid == 0) {
            execv(job->jobName, job->args);//if it go wait needs to free itself and needs to free job when its finished
            perror(SYS_CALL_ERR);
            deleteJob(job);
            exit(1);//or exit 0?
        }
        else if (pid > 0) {
            job->pid = pid;
            printf("%d\n", pid);
            addJob(jobsQueue, job); //Job *job = getPromptJob(&wait);
            checkForWait(wait_, pid);
            removeJob(jobsQueue, pid);//change function to accept job
        }
        else if (pid < 0) {
            deleteJob(job);
            perror(UNSUCCESSFUL_FORK);
            //printError(UNSUCCESSFUL_FORK);
        }
    } while (1);
    wait(NULL);//kill instead of wait
    freeJobsQueue(jobsQueue);
}

Job *newJob(char *n_jobName, char *n_args[]) {
    Job *job = (Job *)malloc(sizeof(Job));
    if (!job) {
        free(n_jobName);
        freeArgs(n_args);
        perror(BAD_ALLOC);
        return NULL;
    }
    job->jobName = n_jobName;
    cpyArgs(job, n_args);
    job->next = NULL;
}
void cpyArgs(Job *job, char *n_args[]) {
    int i = 0;
        char *curr = n_args[i];
        while (curr) {
            (job->args)[i] = curr;
            curr = n_args[i++];
    }
    (job->args)[i] = curr;
}
void freeArgs(char *args[]) {
    int i = 0;
    char *curr = args[i];
    while (curr) {
        free(curr);
        curr = args[i++];
    }
}
void deleteJob(Job *job) {
    if (!job) return;
    free(job->jobName);
    freeArgs(job->args);
    free(job);
}
JobsQueue *createJobsQueue() {
    JobsQueue *jobsQueue = (JobsQueue*)malloc(sizeof(JobsQueue));
    if (!jobsQueue) return NULL;
    jobsQueue->first = NULL;
    jobsQueue->secondToLast = NULL;
    jobsQueue->last = NULL;
    jobsQueue->size = 0;
    return jobsQueue;
}
JobsQueue *createJobQueue(Job *job) {
    if (!job) return createJobsQueue();
    JobsQueue *jobsQueue = (JobsQueue*)malloc(sizeof(JobsQueue));
    if (!jobsQueue) return NULL;
    jobsQueue->first = job;
    jobsQueue->secondToLast = job;
    jobsQueue->last = job;
    jobsQueue->size = 1;
    return jobsQueue;
}
int isEmpty(JobsQueue *jobsQueue) { return jobsQueue->size == 0; }
JobsQueue *addJob(JobsQueue *jobsQueue, Job *job) {
    if (!jobsQueue) {
        if (job) freeJobsQueue(createJobQueue(job));
        return jobsQueue;
    }
    if (!job) return NULL;
    if (jobsQueue->size == 0) {
        freeJobsQueue(jobsQueue);
        return createJobQueue(job);
    }
    jobsQueue->secondToLast = jobsQueue->last;
    jobsQueue->last = job;
    jobsQueue->secondToLast->next = jobsQueue->last;
    (jobsQueue->size)++;
    return jobsQueue;
}
void removeFirst(JobsQueue *jobsQueue) {
    if (!jobsQueue || isEmpty(jobsQueue)) return;
    Job *temp = jobsQueue->first;
    jobsQueue->first = jobsQueue->first->next;
    deleteJob(temp);
    (jobsQueue->size)--;
}
void removeJob(JobsQueue *jobsQueue, pid_t pid) {
    if (!jobsQueue || isEmpty(jobsQueue)) return;
    if (jobsQueue->first->pid == pid) {
        removeFirst(jobsQueue);
        return;
    }
    Job *curr = jobsQueue->first;
    Job *next = curr->next;
    while (next && next->pid != pid) {
        curr = next;
        next = next->next;
    }
    if (!next) return; //didn't find pid
    curr->next = next->next;
    if (next == jobsQueue->secondToLast) jobsQueue->secondToLast = curr;
    if (next == jobsQueue->last) updateLastIndex(jobsQueue);
    deleteJob(next);
    (jobsQueue->size)--;
}
void updateLastIndex(JobsQueue *jobsQueue) {
    Job *curr = jobsQueue->first;
    Job *next = curr->next;
    while (next != jobsQueue->secondToLast) {
        curr = next;
        next = next->next;
    }
    jobsQueue->secondToLast = curr;
    jobsQueue->last = next;
}
void freeJobsQueue(JobsQueue *jobsQueue) {
    if (!jobsQueue) return;
    Job *curr = jobsQueue->first;
    while (curr != NULL) {
        Job *temp = curr;
        curr = curr->next;
        deleteJob(temp);
    }
    free(jobsQueue);
}

Job *getPromptJob(int *wait) {
    printf("prompt>");
    char *jobString = (char *)malloc(MAX_JOB_LEN);//check bad alloc
    if (!jobString) {
        perror(BAD_ALLOC);
        return NULL;
    }
    fgets(jobString, MAX_JOB_LEN, stdin);
    jobString[strlen(jobString) - 2] = 0;

    char *args[MAX_ARGS];
    const char space[2] = " ";
    int i = 0;
    char *token = strtok(jobString, space);
    args[i++] = token;//might be bug here, CHECK!!
    /* walk through other tokens */
    while(token) {
        token = strtok(NULL, space);
        args[i++] = token;//might be bug here, CHECK!!
    }//remove backslash n
    if (i >= 1) *wait = (strcmp(args[i - 2], "&") == 0);
    Job *job = newJob(args[0], &args[1]);
    if (!job) return NULL;
    return job;
}

void printError(char *error) {
    fprintf( stderr, error);
    fprintf( stderr, "\n");
}
void checkForWait(int wait, pid_t pid) {
    if (!wait) return;
    waitpid(pid, NULL, 0);
}
void exitPrompt(char *error) {
    //printError(error);
    perror(error);
    //free all memory
    exit(1);//exit status for error?
}