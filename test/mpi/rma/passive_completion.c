/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include "mpitest.h"

// #define VERBOSE

enum Direction{
    DIR_PUT=0,
    DIR_GET,
    NUM_DIRS
};

typedef enum Direction Direction;

enum FlushType{
    NO_FLUSH = 0,
    FLUSH,
    FLUSH_ALL,
    FLUSH_LOCAL,
    FLUSH_LOCAL_ALL,
    NUM_FLUSH_TYPES
};

typedef enum FlushType FlushType;

enum LockType
{
    LOCK_ONE = 0,
    LOCK_ALL,
    NUM_LOCK_TYPES
};

typedef enum LockType LockType;

#define NUM_FLUSH_INTERVALS 3


void
set_buffer_zero(int N, int* buf)
{
    for (int i = 0; i < N; ++i)
    {
        buf[i] = 0;
    }
}

void
set_buffer_counting(int N, int* buf)
{
    for (int i = 0; i < N; ++i)
    {
        buf[i] = i;
    }
}

int
check_buffer(int N, int* buf)
{
    for (int i = 0; i < N; ++i)
    {
        if ( buf[i] != i ) {
            return 0;
        }
    }
    return 1;
}

typedef struct Buffers{
    int N;
    int* send_buf;
    int* recv_buf;
} Buffers;

Buffers
init_buffers(Direction dir, int origin, int target, int N, int* localbuf, int* winbuf)
{
    int myrank;
    MPI_Comm_rank(MPI_COMM_WORLD, &myrank);

    Buffers buffers = {N, NULL, NULL};
    switch(dir) {
        case DIR_PUT:
            buffers.send_buf=(myrank==origin)?localbuf:NULL;
            buffers.recv_buf=(myrank==target)?winbuf:NULL;
            break;
        case DIR_GET:
            buffers.send_buf=(myrank==target)?winbuf:NULL;
            buffers.recv_buf=(myrank==origin)?localbuf:NULL;
            break;
        default:
            printf("Invalid Direction\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
            return (Buffers){0,NULL,NULL};
    }
    if (buffers.send_buf) set_buffer_counting(buffers.N, buffers.send_buf);
    if (buffers.recv_buf) set_buffer_zero(buffers.N, buffers.recv_buf);
    return buffers;
}

int
check_buffers(int testnum, Buffers buffers)
{
    int myrank;
    MPI_Comm_rank(MPI_COMM_WORLD, &myrank);

    if (buffers.recv_buf)
    {
        if (!check_buffer(buffers.N, buffers.recv_buf))
        {
            printf("Test %d, rank %d: Error in receive buffer\n", testnum, myrank);
            return 0;
        }
    }
    return 1;
}

void
sync_target_with_origin(Direction dir, int target, int origin)
{
    if ( dir == DIR_PUT )
    {
        int myrank;
        MPI_Comm_rank(MPI_COMM_WORLD, &myrank);

        if ( myrank == origin )
        {
            MPI_Send(MPI_BOTTOM, 0, MPI_BYTE, target, 0, MPI_COMM_WORLD);
        }
        else if ( myrank == target )
        {
            MPI_Recv(MPI_BOTTOM, 0, MPI_BYTE, origin, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }
    }
}

void
issue_rma_op(Direction dir, int* buf, int disp, int target, MPI_Win window)
{
    switch(dir)
    {
        case DIR_PUT:
            MPI_Put(buf+disp, 1, MPI_INT, target, disp, 1, MPI_INT, window);
            break;
        case DIR_GET:
            MPI_Get(buf+disp, 1, MPI_INT, target, disp, 1, MPI_INT, window);
            break;
    }
}

void
issue_rma_req_op(Direction dir, int* buf, int disp, int target, MPI_Win window, MPI_Request* req)
{
    switch(dir)
    {
        case DIR_PUT:
            MPI_Rput(buf+disp, 1, MPI_INT, target, disp, 1, MPI_INT, window, req);
            break;
        case DIR_GET:
            MPI_Rget(buf+disp, 1, MPI_INT, target, disp, 1, MPI_INT, window, req);
            break;
    }
}

void issue_flush(FlushType flush, int target, MPI_Win window)
{
    switch(flush)
    {
        case NO_FLUSH:
            /* no-op */
            break;
        case FLUSH:
            MPI_Win_flush(target, window);
            break;
        case FLUSH_ALL:
            MPI_Win_flush_all(window);
            break;
        case FLUSH_LOCAL:
            MPI_Win_flush_local(target, window);
            break;
        case FLUSH_LOCAL_ALL:
            MPI_Win_flush_local_all(window);
            break;
        default:
            printf("Invalid Flush type\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
            break;
    }
}

char*
flush_name(FlushType flush)
{
    switch(flush)
    {
        case NO_FLUSH:
            return "no flush";
        case FLUSH:
            return "MPI_Win_flush";
        case FLUSH_ALL:
            return "MPI_Win_flush_all";
        case FLUSH_LOCAL:
            return "MPI_Win_flush_local";
        case FLUSH_LOCAL_ALL:
            return "MPI_Win_flush_local_all";
        default:
            printf("Invalid Flush type\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
            break;
    }
}


void
issue_lock(LockType lock, int target, MPI_Win window)
{
    switch(lock)
    {
        case LOCK_ONE:
            MPI_Win_lock(MPI_LOCK_EXCLUSIVE, target, 0, window);
            break;
        case LOCK_ALL:
            MPI_Win_lock_all(0, window);
            break;
        default:
            printf("Invalid Lock type\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
            break;
    }
}

void
issue_unlock(LockType lock, int target, MPI_Win window)
{
    switch(lock)
    {
        case LOCK_ONE:
            MPI_Win_unlock(target, window);
            break;
        case LOCK_ALL:
            MPI_Win_unlock_all(window);
            break;
        default:
            printf("Invalid Lock type\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
            break;
    }
}


int
wait_all_outstanding(int current, int last_completed, MPI_Request* requests)
{
    const int num_requests = current-last_completed;
    if (num_requests <= 0)
    {
        return 1;
    }

    MPI_Status statuses[num_requests];
    MPI_Waitall(current-last_completed, requests+last_completed+1, statuses);
    for ( int i = 0; i < num_requests; ++i)
    {
        if ( statuses[i].MPI_ERROR != MPI_SUCCESS )
        {
            printf("Error in Status[%d]: %d\n",i, statuses[i].MPI_ERROR);
            return 0;
        }
    }
    return 1;
}

int
all_requests_completed(int N, MPI_Request* requests)
{
    for (int i = 0; i < N; ++i)
    {
        if ( requests[i] != MPI_REQUEST_NULL )
        {
            printf("Request %d was not completed\n", i);
            return 0;
        }
    }
    return 1;
}


int main(int argc, char *argv[])
{
    int test_sets[4] = {0, 0, 0, 0};
    for (int i = 1; i < argc; ++i)
    {
        int n = atoi(argv[i]);
        if ( n >= 1 && n <= 4)
        {
            test_sets[n-1] = 1;
        }
    }

    int rank, nproc, i;
    int errors = 0;

    const int N = 100;
    int buf[N];
    int winbuf[N];
    MPI_Win window;

    MTest_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nproc);

    if (nproc < 2) {
        if (rank == 0)
            printf("Error: must be run with two or more processes\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    MPI_Win_create(winbuf, N * sizeof(int), sizeof(int), MPI_INFO_NULL,
                   MPI_COMM_WORLD, &window);

    const int origin = 1;
    const int target = 0;

    int testnum = 0;

    int flush_intervals[NUM_FLUSH_INTERVALS] = {1, 3, N+1};

    /* Implicit requests: MPI_Put, MPI_Get */
    if ( test_sets[0] )
    {
        for ( LockType lock = 0; lock < NUM_LOCK_TYPES; ++lock)
        {
            for ( FlushType flush = 0; flush < NUM_FLUSH_TYPES; ++flush)
            {
                for ( int fidx = 0; fidx < NUM_FLUSH_INTERVALS; ++fidx)
                {
                    int flush_interval = flush_intervals[fidx];
                    for ( Direction dir = 0; dir < NUM_DIRS; ++dir)
                    {
                        ++testnum;
                        #if defined(VERBOSE)
                        if (rank == 0)
                        {
                            printf("Test %d\n", testnum);
                            printf("MPI_Win_lock%s, iterate(%s from %d to %d, %s every %d operations), MPI_Win_unlock%s\n",
                                    (lock == LOCK_ONE)?"":"_all",
                                   (dir == DIR_GET)?"MPI_Get":"MPI_Put",
                                    origin,
                                    target,
                                    flush_name(flush),
                                    flush_interval,
                                    (lock == LOCK_ONE)?"":"_all");
                        }
                        #endif
                        Buffers buffers = init_buffers(dir, origin, target, N, buf, winbuf);

                        if (rank == origin )
                        {
                            issue_lock(lock, target, window);
                            for ( int i = 0; i < N; ++i)
                            {
                                issue_rma_op(dir, buf, i, target, window);
                                if ( i%flush_interval == flush_interval-1 )
                                {
                                    issue_flush(flush, target, window);
                                }
                            }
                            issue_unlock(lock, target, window);
                        }
                        sync_target_with_origin(dir, target, origin);
                        if (!check_buffers(testnum, buffers)) {++errors;}
                        MPI_Barrier(MPI_COMM_WORLD);
                    }
                }
            }
        }
    }


    /* Explicit requests, MPI_Rput, MPI_Rget */
    if ( test_sets[1] )
    {
        for ( LockType lock = 0; lock < NUM_LOCK_TYPES; ++lock)
        {
            for ( FlushType flush = 0; flush < NUM_FLUSH_TYPES; ++flush)
            {
                for ( int fidx = 0; fidx < NUM_FLUSH_INTERVALS; ++fidx)
                {
                    int flush_interval = flush_intervals[fidx];
                    for ( Direction dir = 0; dir < NUM_DIRS; ++dir)
                    {
                        ++testnum;
                        #if defined(VERBOSE)
                        if (rank == 0)
                        {
                            printf("Test %d\n", testnum);
                            printf("MPI_Win_lock%s, iterate(%s from %d to %d, MPI_Wait and %s every %d operations), MPI_Wait and MPI_Win_unlock%s\n",
                                    (lock == LOCK_ONE)?"":"_all",
                                   (dir == DIR_GET)?"MPI_Rget":"MPI_Rput",
                                    origin,
                                    target,
                                    flush_name(flush),
                                    flush_interval,
                                    (lock == LOCK_ONE)?"":"_all");
                        }
                        #endif
                        Buffers buffers = init_buffers(dir, origin, target, N, buf, winbuf);

                        if (rank == origin )
                        {
                            MPI_Request requests[N];
                            int last_completed = -1;
                            issue_lock(lock, target, window);
                            for ( int i = 0; i < N; ++i)
                            {
                                issue_rma_req_op(dir, buf, i, target, window, &requests[i]);
                                if ( i%flush_interval == flush_interval-1 )
                                {
                                    if ( !wait_all_outstanding(i, last_completed, requests) ) { ++errors;}
                                    last_completed = i;
                                    issue_flush(flush, target, window);
                                }
                            }
                            if ( !wait_all_outstanding(N-1, last_completed, requests) ) { ++errors;}
                            if ( !all_requests_completed(N, requests) ) {++errors;}
                            issue_unlock(lock, target, window);
                        }
                        sync_target_with_origin(dir, target, origin);
                        if (!check_buffers(testnum, buffers)) {++errors;}
                        MPI_Barrier(MPI_COMM_WORLD);
                    }
                }
            }
        }
    }

    /* Explicit requests, MPI_Rput, MPI_Rget, completion after Window sync */
    if ( test_sets[2] )
    {
        for ( LockType lock = 0; lock < NUM_LOCK_TYPES; ++lock)
        {
            for ( FlushType flush = 0; flush < NUM_FLUSH_TYPES; ++flush)
            {
                for ( int fidx = 0; fidx < NUM_FLUSH_INTERVALS; ++fidx)
                {
                    int flush_interval = flush_intervals[fidx];
                    for ( Direction dir = 0; dir < NUM_DIRS; ++dir)
                    {
                        ++testnum;
                        #if defined(VERBOSE)
                        if (rank == 0)
                        {
                            printf("Test %d\n", testnum);
                            printf("MPI_Win_lock%s, iterate(%s from %d to %d, %s and MPI_Wait every %d operations), MPI_Win_unlock%s and MPI_Wait\n",
                                    (lock == LOCK_ONE)?"":"_all",
                                   (dir == DIR_GET)?"MPI_Rget":"MPI_Rput",
                                    origin,
                                    target,
                                    flush_name(flush),
                                    flush_interval,
                                    (lock == LOCK_ONE)?"":"_all");
                        }
                        #endif
                        Buffers buffers = init_buffers(dir, origin, target, N, buf, winbuf);

                        if (rank == origin )
                        {
                            MPI_Request requests[N];
                            int last_completed = -1;
                            issue_lock(lock, target, window);
                            for ( int i = 0; i < N; ++i)
                            {
                                issue_rma_req_op(dir, buf, i, target, window, &requests[i]);
                                if ( i%flush_interval == flush_interval-1 )
                                {
                                    issue_flush(flush, target, window);
                                    if ( !wait_all_outstanding(i, last_completed, requests) ) { ++errors;}
                                    last_completed = i;
                                }
                            }
                            issue_unlock(lock, target, window);
                            if ( !wait_all_outstanding(N-1, last_completed, requests) ) { ++errors;}
                            if ( !all_requests_completed(N, requests) ) {++errors;}
                        }
                        sync_target_with_origin(dir, target, origin);
                        if (!check_buffers(testnum, buffers)) {++errors;}
                        MPI_Barrier(MPI_COMM_WORLD);
                    }
                }
            }
        }
    }

    /* Explicit requests,  (Lock, MPI_Rput|MPI_Rget, Unlock) x N, then complete all */
    if ( test_sets[3] )
    {
        for ( LockType lock = 0; lock < NUM_LOCK_TYPES; ++lock)
        {
            for ( Direction dir = 0; dir < NUM_DIRS; ++dir)
            {
//                 LockType lock = LOCK_ALL;
//                 Direction dir = DIR_GET;
                ++testnum;
                #if defined(VERBOSE)
                if (rank == 0)
                {
                    printf("Test %d\n", testnum);
                    printf("iterate(MPI_Win_lock%s, %s from %d to %d, MPI_Win_unlock%s), MPI_Wait\n",
                            (lock == LOCK_ONE)?"":"_all",
                            (dir == DIR_GET)?"MPI_Rget":"MPI_Rput",
                            origin,
                            target,
                            (lock == LOCK_ONE)?"":"_all");
                }
                #endif
                Buffers buffers = init_buffers(dir, origin, target, N, buf, winbuf);

                if (rank == origin )
                {
                    MPI_Request requests[N];
                    for ( int i = 0; i < N; ++i)
                    {
                        issue_lock(lock, target, window);
                        issue_rma_req_op(dir, buf, i, target, window, &requests[i]);
                        issue_unlock(lock, target, window);
                    }
                    if ( !wait_all_outstanding(N-1, -1, requests) ) { ++errors;}
                    if ( !all_requests_completed(N, requests) ) {++errors;}
                }
                sync_target_with_origin(dir, target, origin);
                if (!check_buffers(testnum, buffers)) {++errors;}
                MPI_Barrier(MPI_COMM_WORLD);
            }
        }
    }

    MPI_Win_free(&window);

    MTest_Finalize(errors);

    return MTestReturnValue(errors);
}
