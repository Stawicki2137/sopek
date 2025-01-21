#include "common.h"
typedef unsigned int UINT;
typedef struct sigargst
{
    pthread_t tid;
    int *p_do_work;
    pthread_mutex_t *p_do_workmx;
    sigset_t *pMask;

} sigargs_t;

typedef struct threadargs
{
    pthread_t tid;
    UINT seed;
    int *p_do_work;
    pthread_mutex_t *p_do_workmx;
    int *pDzialka;             // tablica reprezentujaca dzialki
    pthread_mutex_t *mutexes;  // tablica mutexow do dzialek
    int n;
} worker_t;
void *signal_handling(void *);
void *workerFunc(void *);
void ReadArguments(int argc, char **argv, int *n, int *q)
{
    if (argc == 3)
    {
        *n = atoi(argv[1]);
        if (*n < 1 || *n > 20)
        {
            usage(argc, argv);
        }
        *q = atoi(argv[2]);
        if (*q < 1 || *q > 10)
        {
            usage(argc, argv);
        }
    }
    else
        usage(argc, argv);
}

int main(int argc, char *argv[])
{
    srand(time(NULL));

    int do_work = 1;
    pthread_mutex_t do_work_mutex = PTHREAD_MUTEX_INITIALIZER;
    sigset_t oldMask, newMask;
    sigemptyset(&newMask);
    sigaddset(&newMask, SIGINT);
    if (pthread_sigmask(SIG_BLOCK, &newMask, &oldMask))
        ERR("SIG_BLOCK error");

    sigargs_t sigargs;
    sigargs.p_do_work = &do_work;
    sigargs.p_do_workmx = &do_work_mutex;
    sigargs.pMask = &newMask;

    if (pthread_create(&sigargs.tid, NULL, signal_handling, &sigargs))
        ERR("Couldn't create signal handling thread!");
    int N, Q;
    ReadArguments(argc, argv, &N, &Q);  // dziala
    pthread_mutex_t *mutexesA = malloc(N * sizeof(pthread_mutex_t));
    if (mutexesA == NULL)
        ERR("malloc");
    for (int i = 0; i < N; i++)
    {
        if (pthread_mutex_init(&mutexesA[i], NULL) != 0)
            ERR("pthread_mutex_init");
    }
    int *Dzialki = malloc(N * sizeof(int));  /// zrobic strukture na niewykorzystane i  juz posyane?
    if (Dzialki == NULL)
        ERR("malloc");
    worker_t *args = (worker_t *)malloc(sizeof(worker_t) * Q);
    if (args == NULL)
        ERR("malloc");
    for (int i = 0; i < Q; i++)
    {
        args[i].p_do_work = &do_work;
        args[i].p_do_workmx = &do_work_mutex;
        args[i].pDzialka = Dzialki;
        args[i].mutexes = mutexesA;
        args[i].n = N;
        args[i].seed = rand();
    }
    for (int i = 0; i < Q; i++)
    {
        if (pthread_create(&args[i].tid, NULL, workerFunc, (void *)&args[i]))
            ERR("pthread_create");
    }

    if (pthread_join(sigargs.tid, NULL))
        ERR("Can't join with 'signal handling' thread");
    for (int i = 0; i < Q; i++)
    {
        pthread_join(args[i].tid, NULL);
    }
    if (pthread_sigmask(SIG_UNBLOCK, &newMask, &oldMask))
        ERR("SIG_BLOCK error");
    for (int i = 0; i < N; i++)
    {
        if (pthread_mutex_destroy(&mutexesA[i]) != 0)
            ERR("pthread_mutex_destroy");
    }
    free(mutexesA);
    free(Dzialki);
    free(args);

    exit(EXIT_SUCCESS);
}
void *workerFunc(void *voidArgs)
{
    worker_t *args = (worker_t *)voidArgs;
    while (*(args->p_do_work))
    {
        int pnrDzialki = rand_r(&args->seed) % args->n;
        msleep(5 + pnrDzialki);
        pthread_mutex_lock(&args->mutexes[pnrDzialki]);
        args->pDzialka[pnrDzialki] += 5;
        pthread_mutex_unlock(&args->mutexes[pnrDzialki]);
    }
    return NULL;
}
void *signal_handling(void *voidArgs)
{
    sigargs_t *args = voidArgs;
    int signo;

    while (*(args->p_do_work))
    {
        if (sigwait(args->pMask, &signo))
            ERR("sigwait failed");
        switch (signo)
        {
            case SIGINT:
                pthread_mutex_lock(args->p_do_workmx);
                *(args->p_do_work) = 0;
                pthread_mutex_unlock(args->p_do_workmx);
                break;
            default:
                printf("unexpected signal %d\n", signo);
                exit(1);
        }
    }
    return NULL;
}
