#include <stdio.h>
#include <stdlib.h>

typedef struct {
    int pid;
    struct Job *next;
} Job;

typedef struct {
    Job *first;
    Job *last;
    Job *secondToLast;
    int size;
} JobsQueue;

Job *newJob(int n_pid) {
    Job *job = (Job *)malloc(sizeof(Job));
    if ()
    job->pid = n_pid;
    job->next = NULL;
}
JobsQueue *createJobQueue(Job *job) {
    if (!job) return NULL;
    JobsQueue *jobsQueue = (JobsQueue*)malloc(sizeof(JobsQueue));
    if (!jobsQueue) return NULL;
    jobsQueue->first = job;
    jobsQueue->secondToLast = job;
    jobsQueue->last = job;
    jobsQueue->size = 1;
}
void addJob(JobsQueue *jobsQueue, Job *job) {
    if (!job || !jobsQueue) return;
    jobsQueue->secondToLast = jobsQueue->last;
    jobsQueue->last = job;
    (jobsQueue->size)++;
}
void removeJob(JobsQueue *jobsQueue) {
    Job *temp = jobsQueue->first;
    jobsQueue->first = (Job *)jobsQueue->first->next;
    free(temp);
    (jobsQueue->size)--;
}
void freeJobsQueue(JobsQueue *jobsQueue) {
    Job *curr = jobsQueue->first;
    while (curr != NULL) {
        Job *temp = curr;
        curr = curr->next;
        free(temp);
    }
}
int isEmpty(JobsQueue *jobsQueue) { return jobsQueue->size == 0; }

char* getPromptJob();

int main() {
    JobsQueue *jobsQueue = (JobsQueue*)malloc(sizeof(JobsQueue));
    char *promptJob = getPromptJob();
    Job *job = (Job *)job.newJob()
    while (1) {

    }
}