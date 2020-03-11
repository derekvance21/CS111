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
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
int sync_m = 0;
int sync_s = 0;
int sync_lock = 0;
int iterations = 1;
int nthreads = 1;
SortedList_t *list;
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

SortedListElement_t *SortedList_init() {
    SortedList_t *head = (SortedList_t*)malloc(sizeof(SortedList_t));
    head->prev = head;
    head->next = head;
    head->key = NULL;
    return head;
}


void *thread_func(void *arg) {
    SortedListElement_t* targ = (SortedListElement_t*)arg;
    int i;
    for (i = 0; i < iterations; i++) {
        SortedList_insert(list, &(targ[i]));
    }
    i = SortedList_length(list);
    for (i = 0; i < iterations; i++) {
        SortedListElement_t *del = SortedList_lookup(list, targ[i].key);
        if (del)
            SortedList_delete(del);
        else {
            fprintf(stderr, "lookup return NULL\n");
            exit(2);
        }
    }
    return NULL;
}

void *thread_func_s(void *arg) {

    SortedListElement_t* targ = (SortedListElement_t*)arg;
    int i;
    while(__sync_lock_test_and_set(&sync_lock, 1)) ; // spin
    for (i = 0; i < iterations; i++) {
        SortedList_insert(list, &targ[i]);
    }
    __sync_lock_release(&sync_lock);

    while(__sync_lock_test_and_set(&sync_lock, 1)) ; // spin
    i = SortedList_length(list);
    if (i < 0) {
        fprintf(stderr, "length is less than 0\n");
        exit(2);
    }
    __sync_lock_release(&sync_lock);
    
    while(__sync_lock_test_and_set(&sync_lock, 1)) ; // spin
    for (i = 0; i < iterations; i++) {
        SortedListElement_t *del = SortedList_lookup(list, targ[i].key);
        if (del)
            SortedList_delete(del);
        else {
            fprintf(stderr, "lookup return NULL\n");
            exit(2);
        }
    }
    __sync_lock_release(&sync_lock);

    return NULL;
}

void *thread_func_m(void *arg) {

    SortedListElement_t* targ = (SortedListElement_t*)arg;
    
    int i;
    pthread_mutex_lock(&lock);
    for (i = 0; i < iterations; i++) {
        SortedList_insert(list, &targ[i]);
    }
    pthread_mutex_unlock(&lock);
    pthread_mutex_lock(&lock);
    i = SortedList_length(list);
    if (i < 0) {
        fprintf(stderr, "length is less than 0\n");
        exit(2);
    }
    pthread_mutex_unlock(&lock);
    pthread_mutex_lock(&lock);
    for (i = 0; i < iterations; i++) {
        SortedListElement_t *del = SortedList_lookup(list, targ[i].key);
        if (del) {
            SortedList_delete(del);
        }
        else {
            fprintf(stderr, "lookup return NULL\n");
            exit(2);
        }
    }
    pthread_mutex_unlock(&lock);

    return NULL;
}

int main(int argc, char** argv) {
    signal(SIGSEGV, catchSignal);
    void*(*mythread)(void*) = &thread_func;

    static struct option long_options[] = {
        {"iterations", required_argument, NULL, 1 },
        {"threads", required_argument, NULL, 2 },
        {"yield", required_argument, NULL, 3 },
        {"sync", required_argument, NULL, 4 },
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
            default:
                fprintf(stderr, "Unrecognized option: %s\n", argv[optind - 1]);
                exit(1);
        }
    }
    if (optind < argc) {
        fprintf(stderr, "Unrecognized argument: %s\n", argv[optind]);
        exit(1);
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
    list = SortedList_init();

    for (i = 0; i < nthreads; i++) {
        if(pthread_create(&threads[i], NULL, mythread, (void*)&elements[i * iterations]))
            error(strerror(errno));
    }


    for (i = 0; i < nthreads; i++) {
        if(pthread_join(threads[i], NULL))
            error(strerror(errno));
    }

    int list_len = SortedList_length(list);

    if (list_len != 0) {
        fprintf(stderr, "list length is not 0 at end: %d\n", list_len);
    }

    clock_gettime(CLOCK_REALTIME, &finish); 
    long time_nsec = (finish.tv_sec - start.tv_sec) * (long)1e9 + (finish.tv_nsec - start.tv_nsec);

    free(elements);
    free(threads);

    char name[15];
    strcpy(name, "list-");
    strcat(name, yield_args);
    strcat(name, "-");
    strcat(name, sync_arg);
    long operations = nelements * 3;
    printf("%s,%d,%d,1,%ld,%ld,%ld\n", name, nthreads, iterations, operations, time_nsec, time_nsec / operations);
    
    return 0;
}