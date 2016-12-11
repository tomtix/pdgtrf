#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <assert.h>
#include <mpi.h>
#include <unistd.h>

#include "util.h"
#include "getrf.h"
#include "gesv.h"
#include "proc.h"
#include "perf/perf.h"

typedef struct tdp_trf_time_ {
    perf_t dist_time;
    perf_t alloc_time;
    perf_t rand_time;
    perf_t compute_time;
    perf_t total_time;
} tdp_trf_time;

static void print_proc_info(tdp_proc *proc)
{
    char hostname[1024];
    gethostname(hostname, 1024);
    hostname[1023] = 0;
    printf("#rank=%d; group_size=%d; node hostname='%s'\n",
           proc->rank, proc->group_size, hostname);
}

static void trf_rand_matrix(tdp_proc *proc, int N, int b, tdp_trf_time *time)
{
    perf_t p1, p2, p3, p4;
    tdp_trf_dist dist;

    printf("dist START\n");
    perf(&p1);
    tdp_trf_dist_snake(&dist, N, b, proc);
    perf(&time->dist_time);
    printf("dist END\nalloc START\n");

    perf(&p2);
    double *A = tdp_matrix_new(N, b*dist.local_block_count);
    perf(&time->alloc_time);

    printf("alloc END\nr=%d BC=%ld\nA=%p size=%luMB\nrand START\n",
           proc->rank, dist.local_block_count, A,
           (N*b*dist.local_block_count*8UL) / (1024*1024));

    perf(&p3);
    tdp_matrix_rand(N, b*dist.local_block_count, A, -1.0, 1.0);
    perf(&time->rand_time);
    printf("rand END\ncompute START\n");

    perf(&p4);
    tdp_pdgetrf_nopiv(N, A, N, b, &dist, proc);
    perf(&time->compute_time);
    printf("compute END\n");

    // ----------------
    time->total_time = time->compute_time;

    perf_diff(&p1, &time->dist_time);
    perf_diff(&p2, &time->alloc_time);
    perf_diff(&p3, &time->rand_time);
    perf_diff(&p4, &time->compute_time);
    perf_diff(&p1, &time->total_time);
}


#define REDUCE_PRINT_TIME(v_, t_) do {                          \
        uint64_t tmp_ = PERF_MICRO((t_)-> PASTE(v_, _time));    \
        MPI_Reduce(&tmp_, &(v_), 1, MPI_UNSIGNED_LONG,          \
                   MPI_MAX, 0, MPI_COMM_WORLD);                 \
        if (!rank) {                                            \
            printf(#v_"_time=%lu µs || %g s\n",                 \
                   v_, (double) v_ / 1000000UL);                \
        }                                                       \
    } while(0)


static void print_time(int rank, tdp_trf_time *time, int N)
{
    uint64_t compute, rand, dist, alloc, total;
    if (!rank)
        puts("");
    REDUCE_PRINT_TIME(dist, time);
    REDUCE_PRINT_TIME(alloc, time);
    REDUCE_PRINT_TIME(rand, time);
    REDUCE_PRINT_TIME(compute, time);
    REDUCE_PRINT_TIME(total, time);

    if (!rank) {
        puts("");
        printf("MFLOPS=%g\n", PERF_MFLOPS2( compute, (2.0/3.0)*CUBE(N)));
        puts("");
    }
}

int main(int argc, char *argv[])
{
    srand(time(NULL)+(long)&argc);
    MPI_Init(NULL, NULL);

    tdp_proc proc;
    tdp_trf_time time;

    MPI_Comm_size(MPI_COMM_WORLD, &proc.group_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &proc.rank);
    print_proc_info(&proc);

    int N = 2000, b = 200;
    trf_rand_matrix(&proc, N, b, &time);

    print_time(proc.rank, &time, N);

    MPI_Finalize();
    return EXIT_SUCCESS;
}
