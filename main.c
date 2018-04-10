#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <wait.h>
#include <string.h>

#define MAX_JOB_LEN 1024
#define MAX_ARGS 20
#define UNSUCCESSFUL_FORK "Unsuccessful fork"
#define BAD_ALLOC "Bad memory allocation"

typedef struct {
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

Job *newJob(char *jobString);
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
    int wait;
    JobsQueue *jobsQueue = createJobsQueue();
    if (!jobsQueue) exitPrompt(BAD_ALLOC);
    do {
        Job *job = getPromptJob(&wait);
        pid_t pid = fork(); //will only get legal job
        if (pid == 0) {
            execv(job->jobName, job->args);//if it go wait needs to free itself and needs to free job when its finished
            exit(1);//or exit 0?
        }
        else if (pid > 0) {
            job->pid = pid;
            printf("%d\n", pid);
            addJob(jobsQueue, job); //Job *job = getPromptJob(&wait);
            checkForWait(wait, pid);
            removeJob(jobsQueue, pid);//change function to accept job
        }
        else if (pid == -1) {
            deleteJob(job);
            printError(UNSUCCESSFUL_FORK);
        }
    } while (1);
}

Job *newJob(char *jobString) {
    Job *job = (Job *)malloc(sizeof(Job));
    if (!job) {
        free(jobString);
        return NULL;
    }
    job->jobStr = jobString;
    job->next = NULL;
}
void deleteJob(Job *job) {
    if (!job) return;
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
    (Job *)jobsQueue->secondToLast->next = jobsQueue->last;
    (jobsQueue->size)++;
    return jobsQueue;
}
void removeFirst(JobsQueue *jobsQueue) {
    if (!jobsQueue || isEmpty(jobsQueue)) return;
    Job *temp = jobsQueue->first;
    jobsQueue->first = (Job *)jobsQueue->first->next;
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
    Job *next = (Job *)curr->next;
    while (next && next->pid != pid) {
        curr = next;
        next = (Job *)next->next;
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
    Job *next = (Job *)curr->next;
    while (next != jobsQueue->secondToLast) {
        curr = next;
        next = (Job *)next->next;
    }
    jobsQueue->secondToLast = curr;
    jobsQueue->last = next;
}
void freeJobsQueue(JobsQueue *jobsQueue) {
    if (!jobsQueue) return;
    Job *curr = jobsQueue->first;
    while (curr != NULL) {
        Job *temp = curr;
        curr = (Job *)curr->next;
        deleteJob(temp);
    }
    free(jobsQueue);
}

Job *getPromptJob(int *wait) {
    printf("prompt>");
    char *jobString = (char *)malloc(MAX_JOB_LEN);//check bad alloc
    if (!jobString) return NULL;////DON'T DO THIS!!!!!
    fgets(jobString, MAX_JOB_LEN, stdin);
    Job *job = newJob(jobString);
    if (!job) return NULL;

    char *args[MAX_ARGS];
    const char space[2] = " ";
    int i = 0;
    char *token = strtok(jobString, space);
    args[i++] = token;//might be bug here, CHECK!!
    /* walk through other tokens */
    while(token) {
        token = strtok(NULL, space);
        args[i++] = token;//might be bug here, CHECK!!
    }
    if (i >= 1) *wait = (strcmp(args[i], "&") == 0);
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
    printError(error);
    exit(1);//exit status for error?
}