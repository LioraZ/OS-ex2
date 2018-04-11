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
#define MAX_PATH_SIZE 100


typedef struct Job {
    pid_t pid;
    char *jobName;
    char *args[MAX_ARGS];
    struct Job *next;
} Job;

typedef struct {
    Job *first;
    Job *last;
    int size;
} JobsQueue;

/**
 * The function creates a new job given its parameters.
 * @param n_jobName the name of the job.
 * @return a pointer to the newly created job.
 */
Job *newJob(char *n_jobName, char *[]);
/**
 * The function copy's a job's args to the job.
 * @param job The given job.
 * @param n_args The job's args to copied.
 */
void cpyArgs(Job *job, char *n_args[]);
/**
 * The function frees the job's args.
 * @param args The args to free.
 */
void freeArgs(char *args[]);
/**
 * The function deletes the given job.
 * @param job The given job.
 */
void deleteJob(Job *job);
/**
 * The function creates a new JobQueue.
 * @return The new jobQueue.
 */
JobsQueue *createJobsQueue();
/**
 * The function creates a new jobsQueue given a job.
 * @param The given job.
 * @return The new jobsQueue.
 */
JobsQueue *createJobQueue(Job *job);
/**
 * The function returns 1 if the jobsQueue is empty and 0 else.
 * @param jobsQueue The given jobsQueue.
 * @return 1 if the jobsQueue is empty and 0 else.
 */
int isEmpty(JobsQueue *jobsQueue);
/**
 * The function add a new job to the jobsQueue.
 * @param jobsQueue THe given jobsQueue.
 * @param job The given job.
 * @return The jobsQueue.
 */
JobsQueue *addJob(JobsQueue *jobsQueue, Job *job);
/**
 * The function frees the jobsQueue.
 * @param jobsQueue The jobsQueue.
 */
void freeJobsQueue(JobsQueue *jobsQueue);
/**
 * The function returns a job received from prompt.
 * @param wait Flag to wait for fork to finish.
 * @return The new job.
 */
Job *getPromptJob(int *wait);
/**
 * The function checks if thread pid needs to waited for, and waits for it.
 * @param wait The flag.
 * @param pid The thread.
 */
void checkForWait(int wait, pid_t pid);
/**
 * The function exits the command prompt with an error msg.
 * @param error The error msg.
 */
void exitPrompt(char *error);
/**
 * The function checks the jobs name and will execute specific jobs accordingly.
 * @param job The given job.
 * @param jobsQueue The jobsQueue.
 * @return 1 if should continue or 0 to exec and fork.
 */
int checkJobName(Job *job, JobsQueue *jobsQueue);
/**
 * The function will print the jobs from the jobsQueue.
 * @param jobsQueue The jobsQueue.
 */
void printJobs(JobsQueue *jobsQueue);
/**
 * The function will removed jobs that have completed from the jobsQueue.
 * @param jobsQueue The jobsQueue.
 */
void removeCompletedJobs(JobsQueue *jobsQueue);
/**
 * The function will changeDir according to bash's cd.
 * @param args cd's args.
 * @return success or failure.
 */
int cd(char *args[]);


int main() {
    int wait_;
    JobsQueue *jobsQueue = createJobsQueue();
    if (!jobsQueue) exitPrompt(BAD_ALLOC);
    do {
        Job *job = getPromptJob(&wait_);
        if (!job) break;
        if (checkJobName(job, jobsQueue)) continue;
        pid_t pid = fork();
        if (pid == 0) {
            execvp(job->jobName, job->args);
            perror(SYS_CALL_ERR);
            deleteJob(job);
            exit(1);//or exit 0?
        }
        else if (pid > 0) {
            job->pid = pid;
            printf("%d\n", pid);
            jobsQueue = addJob(jobsQueue, job); //check for null
            checkForWait(wait_, pid);
        }
        else if (pid < 0) {
            deleteJob(job);
            perror(UNSUCCESSFUL_FORK);
            //exit w freeing
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
            (job->args)[i++] = curr;
            curr = n_args[i];
    }
    (job->args)[i] = curr;
}
void freeArgs(char *args[]) {
    int i = 0;
    char *curr = args[i];
    while (curr) {
        char *temp = curr;
        curr = args[i++];
        free(temp);
    }
}
void deleteJob(Job *job) {
    if (!job) return;
    free(job->jobName);
    int i = 0;
    /*char *curr = job->args[i];
    while (curr) {
        char *temp = curr;
        curr = job->args[i++];
        free(temp);
    }*/
    //freeArgs(job->args);
    free(job);
}
JobsQueue *createJobsQueue() {
    JobsQueue *jobsQueue = (JobsQueue*)malloc(sizeof(JobsQueue));
    if (!jobsQueue) return NULL;
    jobsQueue->first = NULL;
    jobsQueue->last = NULL;
    jobsQueue->size = 0;
    return jobsQueue;
}
JobsQueue *createJobQueue(Job *job) {
    if (!job) return createJobsQueue();
    JobsQueue *jobsQueue = (JobsQueue*)malloc(sizeof(JobsQueue));
    if (!jobsQueue) return NULL;
    jobsQueue->first = job;
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
        jobsQueue = createJobQueue(job);
        return jobsQueue;
    }
    jobsQueue->last->next = job;
    jobsQueue->last = job;
    (jobsQueue->size)++;
    return jobsQueue;
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
char *getInput() {
    char *jobString;
    do {
        printf("prompt>");
        jobString = (char *)malloc(MAX_JOB_LEN);//check bad alloc
        if (!jobString) {
            perror(BAD_ALLOC);
            return NULL;
        }
        fgets(jobString, MAX_JOB_LEN, stdin);
    } while (strcmp(jobString, "\n") == 0);
    jobString[strlen(jobString) - 1] = 0;
    return jobString;
}
Job *getPromptJob(int *wait) {
    char *jobString = getInput();
    char *args[MAX_ARGS];
    const char space[2] = " ";
    int i = 0;
    char *token = strtok(jobString, space);
    args[i++] = token;
    while(token) {
        token = strtok(NULL, space);
        args[i++] = token;
    }
    if (i >= 1) *wait = (strcmp(args[i - 2], "&") != 0);
    if (!(*wait)) args[i - 2] = 0;
    Job *job = newJob(args[0], &args[0]);
    if (!job) return NULL;
    return job;
}
void checkForWait(int wait, pid_t pid) {
    if (!wait) return;
    waitpid(pid, NULL, 0);
}
void exitPrompt(char *error) {
    perror(error);
    exit(1);
}
int checkJobName(Job *job, JobsQueue *jobsQueue) {
    char *jobName = job->jobName;
    if (strcmp(jobName, "exit") == 0) {
        freeJobsQueue(jobsQueue);
        exit(1);
    }
    if (strcmp(jobName, "jobs") == 0) {
        removeCompletedJobs(jobsQueue);
        printJobs(jobsQueue);
        return 1;
    }
    if (strcmp(jobName, "cd") == 0) {
        cd(job->args);
        printf("%d\n", getpid());
        return 1;
    }
    return 0;
}

void printJobs(JobsQueue *jobsQueue) {
    Job *job = jobsQueue->first;
    while (job) {
        printf("%d\t", job->pid);
        int i = 0;
        while (job->args[i]) printf("%s ", job->args[i++]);
        printf("\n");
        job = job->next;
    }
}

void removeCompletedJobs(JobsQueue *jobsQueue) {
    Job *curr = jobsQueue->first;
    if (!curr) return;
    Job *next = curr->next;
    while (next) {
        if ((getpgid(next->pid) < 0)) {
            curr->next = next->next;
            deleteJob(next);
        }
        curr = curr->next;
        if (curr) next = curr->next;
        else next = NULL;
    }
    if (jobsQueue->first && (getpgid(jobsQueue->first->pid) < 0)) {
        curr = jobsQueue->first;
        jobsQueue->first = jobsQueue->first->next;
        deleteJob(curr);
    }
}

int cd(char *args[]) {
    char *pth = args[1];//what if no path
    char path[MAX_PATH_SIZE];
    strcpy(path,pth);

    char cwd[MAX_PATH_SIZE];
    if(pth[0] != '/')
    {// true for the dir in cwd
        getcwd(cwd,sizeof(cwd));
        strcat(cwd,"/");
        strcat(cwd,path);
        chdir(cwd);
    }else{//true for dir w.r.t. /
        chdir(pth);
    }
    return 0;
}
