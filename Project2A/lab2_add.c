#include <time.h>
#include <getopt.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>

int opt_yield = 0;
char *sync_arg = NULL;
long long counter = 0;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

int sync_lock = 0;


void error(char* msg) {
    fprintf(stderr, "%s\n", msg);
    exit(1);
}

void add(long long *pointer, long long value) {
    long long sum = *pointer + value;
    if (opt_yield) 
        sched_yield();
    *pointer = sum;
}

void add_m(long long *pointer, long long value) {
    pthread_mutex_lock(&lock);
    add(pointer, value);
    pthread_mutex_unlock(&lock);
}

void add_s(long long *pointer, long long value) {
    while(__sync_lock_test_and_set(&sync_lock, 1))
        ; // spin
    add(pointer, value);
    __sync_lock_release(&sync_lock);
}

void add_c(long long *pointer, long long value) {
    long long old, new;
    do {
        old = *pointer;
        new = old + value;
        if (opt_yield)
            sched_yield();
    } while (__sync_val_compare_and_swap(pointer, old, new) != old);
}

void(*add_func)(long long*, long long) = &add;

void *thread_add(void* arg) {
    int iterations = *(int*)arg; int i;
    for (i = 0; i < iterations; i++) 
        (*add_func)(&counter, 1);
    for (i = 0; i < iterations; i++)
        (*add_func)(&counter, -1);
    return NULL;
}


int main(int argc, char** argv) {
    int nthreads = 1;
    int iterations = 1;

    static struct option long_options[] = {
        {"iterations", required_argument, NULL, 1 },
        {"threads", required_argument, NULL, 2 },
        {"yield", no_argument, NULL, 3 },
        {"sync", required_argument, NULL, 4},
        {0, 0, 0, 0}
    };
    int c;
    while ( (c = getopt_long(argc, argv, "", long_options, NULL)) != -1 ) {
        switch(c) {
            case 1:
                iterations = atoi(optarg);
                break;
            case 2:
                nthreads = atoi(optarg);
                break;
            case 3:
                opt_yield = 1;
                break;
            case 4:
                sync_arg = optarg;
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

    if (sync_arg) {
        switch(*sync_arg) {
            case 'm':
                add_func = &add_m;
                break;
            case 's':
                add_func = &add_s;
                break;
            case 'c':
                add_func = &add_c;
                break;
            default:
                fprintf(stderr, "Unrecognized argument to --sync: %s\n", sync_arg);
                exit(1);
                break;
        }
    }

    struct timespec start, finish; 
    clock_gettime(CLOCK_REALTIME, &start);

    pthread_t *threads = (pthread_t*)malloc(nthreads * sizeof(pthread_t));

    int i;
    for (i = 0; i < nthreads; i++) {
        if(pthread_create(&threads[i], NULL, thread_add, (void*)&iterations))
            error(strerror(errno));
    }

    for (i = 0; i < nthreads; i++) {
        if(pthread_join(threads[i], NULL))
            error(strerror(errno));
    }

    clock_gettime(CLOCK_REALTIME, &finish); 
    long time_nsec = finish.tv_nsec - start.tv_nsec;
    long operations = (long)nthreads * (long)iterations * 2;

    char name[15];
    strcpy(name, "add-");
    if (opt_yield)
        strcat(name, "yield-");
    if (sync_arg)
        strcat(name, sync_arg);
    else
        strcat(name, "none"); 
    
    printf("%s,%d,%d,%ld,%ld,%ld,%lld\n", name, nthreads, iterations, operations, time_nsec, time_nsec / operations, counter);
    return 0;
}