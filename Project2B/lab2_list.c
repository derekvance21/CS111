/*
NAME: Derek Vance
EMAIL: dvance@g.ucla.edu
UID: 604970765
*/

#include "SortedList.h"
#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>


int opt_yield = 0;
pthread_mutex_t* locks;
int *sync_locks;
int iterations = 1;
int nthreads = 1;
int nlists = 1;
SortedList_t **lists;
SortedListElement_t *elements;


void catchSignal(int sig) {
    if (sig == SIGSEGV)
        fprintf(stderr, "Segmentation fault caught\n");
    exit(2);
}

void error(char* msg) {
    fprintf(stderr, "%s\n", msg);
    exit(1);
}

void Pthread_mutex_lock(pthread_mutex_t *mutex) {
    int rc = pthread_mutex_lock(mutex);
    if (rc != 0)
        error("Error locking");
}

void Pthread_mutex_unlock(pthread_mutex_t *mutex) {
    int rc = pthread_mutex_unlock(mutex);
    if (rc != 0)
        error("Error unlocking");
}

SortedListElement_t *SortedList_init() {
    SortedList_t *head = (SortedList_t*)malloc(sizeof(SortedList_t));
    head->prev = head;
    head->next = head;
    head->key = NULL;
    return head;
}

SortedListElement_t **Partitioned_SortedList_init(int num) {
    SortedList_t **listarray = (SortedList_t**)malloc(sizeof(SortedList_t*) * num);
    int i;
    for (i = 0; i < num; i++) {
        listarray[i] = SortedList_init();
    }
    return listarray;
}


void *thread_func(void *arg) {
    SortedListElement_t* targ = (SortedListElement_t*)arg;
    int i;
    for (i = 0; i < iterations; i++) {
        SortedList_insert(lists[*(targ[i].key) % nlists], &(targ[i]));
    }
    int length = 0;
    for (i = 0; i < nlists; i++) {
        length += SortedList_length(lists[i]);
    }
    if (length < 0) {
        fprintf(stderr, "length is less than 0\n");
        exit(2);
    }
    for (i = 0; i < iterations; i++) {
        SortedListElement_t *del = SortedList_lookup(lists[*(targ[i].key) % nlists], targ[i].key);
        if (del)
            SortedList_delete(del);
        else {
            fprintf(stderr, "lookup return NULL\n");
            exit(2);
        }
    }
    long lock_time = 0;
    return (void*)lock_time;
}

void *thread_func_s(void *arg) {
    struct timespec start, finish; 
    long lock_time = 0;

    SortedListElement_t* targ = (SortedListElement_t*)arg;
    int i;
    for (i = 0; i < iterations; i++) {
        int hash = *(targ[i].key) % nlists;
        clock_gettime(CLOCK_REALTIME, &start);
        while(__sync_lock_test_and_set(&sync_locks[hash], 1)) ; // spin
        clock_gettime(CLOCK_REALTIME, &finish);
        lock_time += (finish.tv_sec - start.tv_sec) * (long)1e9 + (finish.tv_nsec - start.tv_nsec);

        SortedList_insert(lists[hash], &(targ[i]));
        __sync_lock_release(&sync_locks[hash]);
    }

    int length = 0;
    for (i = 0; i < nlists; i++) {
        clock_gettime(CLOCK_REALTIME, &start);
        while(__sync_lock_test_and_set(&sync_locks[i], 1)) ; // spin
        clock_gettime(CLOCK_REALTIME, &finish);
        lock_time += (finish.tv_sec - start.tv_sec) * (long)1e9 + (finish.tv_nsec - start.tv_nsec);

        length += SortedList_length(lists[i]);
        __sync_lock_release(&sync_locks[i]);
    }
    if (length < 0) {
        fprintf(stderr, "length is less than 0\n");
        exit(2);
    }
    
    for (i = 0; i < iterations; i++) {
        int hash = *(targ[i].key) % nlists;
        clock_gettime(CLOCK_REALTIME, &start);
        while(__sync_lock_test_and_set(&sync_locks[hash], 1)) ; // spin
        clock_gettime(CLOCK_REALTIME, &finish);
        lock_time += (finish.tv_sec - start.tv_sec) * (long)1e9 + (finish.tv_nsec - start.tv_nsec);

        SortedListElement_t *del = SortedList_lookup(lists[hash], targ[i].key);
        if (del)
            SortedList_delete(del);
        else {
            fprintf(stderr, "lookup return NULL\n");
            exit(2);
        }
        __sync_lock_release(&sync_locks[hash]);
    }

    return (void*)lock_time;
}

void *thread_func_m(void *arg) {
    struct timespec start, finish; 
    long lock_time = 0;

    SortedListElement_t* targ = (SortedListElement_t*)arg;
    int i;

    for (i = 0; i < iterations; i++) {
        int hash = *(targ[i].key) % nlists;
        clock_gettime(CLOCK_REALTIME, &start);
        Pthread_mutex_lock(&locks[hash]);
        clock_gettime(CLOCK_REALTIME, &finish);
        lock_time += (finish.tv_sec - start.tv_sec) * (long)1e9 + (finish.tv_nsec - start.tv_nsec);

        SortedList_insert(lists[hash], &(targ[i]));
        Pthread_mutex_unlock(&locks[hash]);
    }


    int length = 0;
    for (i = 0; i < nlists; i++) {
        clock_gettime(CLOCK_REALTIME, &start);
        Pthread_mutex_lock(&locks[i]);
        clock_gettime(CLOCK_REALTIME, &finish);
        lock_time += (finish.tv_sec - start.tv_sec) * (long)1e9 + (finish.tv_nsec - start.tv_nsec);
        length += SortedList_length(lists[i]);
        Pthread_mutex_unlock(&locks[i]);
    }
    if (length < 0) {
        fprintf(stderr, "length is less than 0\n");
        exit(2);
    }

    for (i = 0; i < iterations; i++) {
        int hash = *(targ[i].key) % nlists;
        clock_gettime(CLOCK_REALTIME, &start);
        Pthread_mutex_lock(&locks[hash]);
        clock_gettime(CLOCK_REALTIME, &finish);
        lock_time += (finish.tv_sec - start.tv_sec) * (long)1e9 + (finish.tv_nsec - start.tv_nsec);
        SortedListElement_t *del = SortedList_lookup(lists[hash], targ[i].key);
        if (del) {
            SortedList_delete(del);
        }
        else {
            fprintf(stderr, "lookup return NULL\n");
            exit(2);
        }
        Pthread_mutex_unlock(&locks[hash]);
    }

    return (void*)lock_time;
}

int main(int argc, char** argv) {
    signal(SIGSEGV, catchSignal);
    void*(*mythread)(void*) = &thread_func;

    static struct option long_options[] = {
        {"iterations", required_argument, NULL, 1 },
        {"threads", required_argument, NULL, 2 },
        {"yield", required_argument, NULL, 3 },
        {"sync", required_argument, NULL, 4 },
        {"lists", required_argument, NULL, 5 },
        {0, 0, 0, 0}
    };
    char *yield_args = "none";
    char *sync_arg = "none";
    int c;
    while ( (c = getopt_long(argc, argv, "", long_options, NULL)) != -1 ) {
        switch(c) {
            case 1:
                iterations = atoi(optarg);
                break;
            case 2:
                nthreads = atoi(optarg);
                break;
            case 3: ;
                unsigned int i = 0;
                yield_args = optarg;
                for (; i < strlen(yield_args); i++) {
                    switch(optarg[i]) {
                        case 'i':
                            opt_yield = opt_yield | INSERT_YIELD;
                            break;
                        case 'd':
                            opt_yield = opt_yield | DELETE_YIELD;
                            break;
                        case 'l':
                            opt_yield = opt_yield | LOOKUP_YIELD;
                            break;
                        default:
                            fprintf(stderr, "Invalid argument to --yield: %c\n", optarg[i]);
                            exit(1);
                            break;
                    }
                }
                break;
            case 4:
                sync_arg = optarg;
                if (strcmp(sync_arg, "s") == 0) {
                    mythread = &thread_func_s;
                }
                else if (strcmp(sync_arg, "m") == 0) {
                    mythread = &thread_func_m;
                }
                else {
                    fprintf(stderr, "Invalid argument to --sync: %s\n", sync_arg);
                    exit(1);
                }
                break;
            case 5:
                nlists = atoi(optarg);
                break;
            default:
                fprintf(stderr, "Unrecognized option: %s\n", argv[optind - 1]);
                exit(1);
        }
    }
    if (optind < argc) {
        fprintf(stderr, "Unrecognized argument: %s\n", argv[optind]);
        exit(1);
    }
    
    if (strcmp(sync_arg, "s") == 0) {
        sync_locks = malloc(nlists * sizeof(int));
        int i;
        for (i = 0; i < nlists; i++)
            sync_locks[i] = 0;
    }

    else if (strcmp(sync_arg, "m") == 0) {
        locks = malloc(nlists * sizeof(pthread_mutex_t));
        int i;
        for (i = 0; i < nlists; i++) {
            int rc = pthread_mutex_init(&locks[i], NULL);
            if (rc != 0) {
                error(strerror(errno));
            }
        }
    }

    long nelements = (long)nthreads * (long)iterations;
    elements = (SortedListElement_t*)malloc(nelements * sizeof(SortedListElement_t));
    char keys[94];
    int i;
    for (i = 0; i < 94; i++)
        keys[i] = 33 + i;    
    for (i = 0; i < nelements; i++)
        elements[i].key = &keys[random() % 94];

    struct timespec start, finish; 
    clock_gettime(CLOCK_REALTIME, &start);

    pthread_t *threads = (pthread_t*)malloc(nthreads * sizeof(pthread_t));
    lists = Partitioned_SortedList_init(nlists);

    for (i = 0; i < nthreads; i++) {
        if(pthread_create(&threads[i], NULL, mythread, (void*)&elements[i * iterations]))
            error(strerror(errno));
    }

    long lock_time;
    long total_lock_time;
    for (i = 0; i < nthreads; i++) {
        if(pthread_join(threads[i], (void**) &lock_time))
            error(strerror(errno));
        total_lock_time += (long)lock_time;
    }

    int list_len = 0;
    for (i = 0; i < nlists; i++) {
        list_len += SortedList_length(lists[i]);
    }
    if (list_len != 0) {
        fprintf(stderr, "list length is not 0 at end: %d\n", list_len);
    }

    clock_gettime(CLOCK_REALTIME, &finish); 
    long time_nsec = (finish.tv_sec - start.tv_sec) * (long)1e9 + (finish.tv_nsec - start.tv_nsec);

    free(elements);
    free(threads);
    free(lists);
    if (strcmp(sync_arg, "m") == 0)
        free(locks);
    else if (strcmp(sync_arg, "s") == 0)
        free(sync_locks);

    char name[15];
    strcpy(name, "list-");
    strcat(name, yield_args);
    strcat(name, "-");
    strcat(name, sync_arg);
    long operations = nelements * 3;
    printf("%s,%d,%d,%d,%ld,%ld,%ld,%ld\n", name, nthreads, iterations, nlists, operations, time_nsec, time_nsec / operations, total_lock_time / operations);
    
    return 0;
}