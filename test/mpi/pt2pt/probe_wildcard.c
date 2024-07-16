/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mpi.h"
#include "mpitest.h"

static int errs = 0;
#define MAX_ASSERT_MESSAGE_LENGTH 256
static char assert_message[MAX_ASSERT_MESSAGE_LENGTH];

#define log_error(msg_) { ++errs; if( errs < 10 ) { fprintf(stderr, "check failed: (%s), line %d\n", msg_, __LINE__); } }

/* assert-like macro that bumps the err count and emits a message */
#define check(x_) if (!(x_)) { log_error(#x_); }

#define assert_int_equals(expected_, actual_, msg_) if (!(expected_ == actual_)) { \
    snprintf(assert_message, MAX_ASSERT_MESSAGE_LENGTH, "%s: expected %d, got %d", msg_, expected_, actual_); \
    log_error(assert_message); \
}

#define assert_status_equals(source_, tag_, error_, count_, type_, status_) \
    do { \
        assert_int_equals(source_, status_.MPI_SOURCE, "status source"); \
        assert_int_equals(tag_, status_.MPI_TAG, "status tag"); \
        assert_int_equals(error_, status_.MPI_ERROR, "status error"); \
        int status_count_; \
        MPI_Get_count(&status_, type_, &status_count_); \
        assert_int_equals(count_, status_count_, "status count"); \
    } while (0)


int buffer_equals(int N, int* expected, int* actual)
{
    for ( int i = 0; i < N; ++i)
    {
        if ( expected[i] != actual[i] )
        {
            return 0;
        }
    }
    return 1;
}

void init_buffers(int* sendbuf, int* recvbuf)
{
    sendbuf[0] = 0x12345678;
    sendbuf[1] = 0xfedcba98;

    recvbuf[0] = 0x0;
    recvbuf[1] = 0x0;
}


int main(int argc, char **argv)
{
    int errs = 0;
    int rank, size;

    MTest_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (size < 2) {
        printf("this test requires at least 2 processes\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    /* all processes besides ranks 0 & 1 aren't used by this test */
    if (rank >= 2) {
        goto epilogue;
    }

    int sendbuf[2];
    int recvbuf[2];

    int sender = 0;
    int receiver = 1;
    int tag = 99;

    MPI_Status recv_status;
    MPI_Status probe_status;

    /* Test 0: MPI_Probe+MPI_Recv */
    init_buffers(sendbuf, recvbuf);
    if ( rank == sender )
    {
        MPI_Send(sendbuf, 2, MPI_INT, receiver, tag, MPI_COMM_WORLD);
    }
    else
    {
        MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &probe_status);
        assert_status_equals(sender, tag, MPI_SUCCESS, 2, MPI_INT, probe_status);

        int recv_count;
        MPI_Get_count(&probe_status, MPI_INT, &recv_count);
        MPI_Recv(recvbuf, recv_count, MPI_INT, probe_status.MPI_SOURCE, probe_status.MPI_TAG, MPI_COMM_WORLD, &recv_status);
        assert_status_equals(sender, tag, MPI_SUCCESS, 2, MPI_INT, recv_status);
        check(buffer_equals(2, sendbuf, recvbuf));
    }

    /* Test 1: MPI_Iprobe+MPI_Recv */
    init_buffers(sendbuf, recvbuf);
    if ( rank == sender )
    {
        MPI_Send(sendbuf, 2, MPI_INT, receiver, tag, MPI_COMM_WORLD);
    }
    else
    {
        int probe_successful = 0;
        while ( !probe_successful )
        {
            MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &probe_successful, &probe_status);
        }
        assert_status_equals(sender, tag, MPI_SUCCESS, 2, MPI_INT, probe_status);

        int recv_count;
        MPI_Get_count(&probe_status, MPI_INT, &recv_count);
        MPI_Recv(recvbuf, recv_count, MPI_INT, probe_status.MPI_SOURCE, probe_status.MPI_TAG, MPI_COMM_WORLD, &recv_status);
        assert_status_equals(sender, tag, MPI_SUCCESS, 2, MPI_INT, recv_status);
        check(buffer_equals(2, sendbuf, recvbuf));
    }


    /* Test 2: MPI_Mprobe+MPI_Mrecv */
    init_buffers(sendbuf, recvbuf);
    if ( rank == sender )
    {
        MPI_Send(sendbuf, 2, MPI_INT, receiver, tag, MPI_COMM_WORLD);
    }
    else
    {
        MPI_Message message;
        MPI_Mprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &message, &probe_status);
        assert_status_equals(sender, tag, MPI_SUCCESS, 2, MPI_INT, probe_status);
        check(message != MPI_MESSAGE_NO_PROC && message != MPI_MESSAGE_NULL);

        int recv_count;
        MPI_Get_count(&probe_status, MPI_INT, &recv_count);
        MPI_Mrecv(recvbuf, recv_count, MPI_INT, &message, &recv_status);
        assert_status_equals(sender, tag, MPI_SUCCESS, 2, MPI_INT, recv_status);
        check(buffer_equals(2, sendbuf, recvbuf));
    }

    /* Test 3: MPI_Improbe+MPI_Mrecv */
    init_buffers(sendbuf, recvbuf);
    if ( rank == sender )
    {
        MPI_Send(sendbuf, 2, MPI_INT, receiver, tag, MPI_COMM_WORLD);
    }
    else
    {
        MPI_Message message;
        int probe_successful = 0;
        while ( !probe_successful )
        {
            MPI_Improbe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &probe_successful, &message, &probe_status);
        }
        assert_status_equals(sender, tag, MPI_SUCCESS, 2, MPI_INT, probe_status);
        check(message != MPI_MESSAGE_NO_PROC && message != MPI_MESSAGE_NULL);

        int recv_count;
        MPI_Get_count(&probe_status, MPI_INT, &recv_count);
        MPI_Mrecv(recvbuf, recv_count, MPI_INT, &message, &recv_status);
        assert_status_equals(sender, tag, MPI_SUCCESS, 2, MPI_INT, recv_status);
        check(buffer_equals(2, sendbuf, recvbuf));
    }
  epilogue:
    MTest_Finalize(errs);

    return MTestReturnValue(errs);
}
