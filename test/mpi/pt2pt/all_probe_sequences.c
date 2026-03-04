#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "mpi.h"
#include "mpitest.h"

#define VERBOSE 0

const int NUM_TESTS = 6;
const int BUF_SCALE = 10;
const int MAX_BUF = BUF_SCALE * NUM_TESTS ;


int buffer_elements(int test)
{
    return ( test + 1 ) * BUF_SCALE;
}


void prepare_send_buf(int num, int* buf)
{
    for ( int i = 0; i < num; ++i )
    {
        buf[i] = 1;
    }
    for ( int i = num; i < MAX_BUF; ++i )
    {
        buf[i] = 0;
    }
}


void reset_recv_buf(int* buf)
{
    for ( int i = 0; i < MAX_BUF; ++i )
    {
        buf[i] = 0;
    }
}

int check_recv_buf(int test, int count, int* buf)
{
    if ( count != buffer_elements(test) )
    {
        printf("Test %d: Expected to receive %d elements, but got %d\n", test, buffer_elements(test), count);
        return 1;
    }
    for ( int i = 0; i < count; ++i )
    {
        if ( buf[i] != 1 )
        {
            printf("Test %d: Recv buffer elements not correct\n", test);
            return 1;
        }
    }
    return 0;
}


void print_status(  const char* name, MPI_Status* status, MPI_Datatype type )
{
    if ( status == MPI_STATUS_IGNORE )
    {
        printf("Status(IGNORED)\n");
    }
    else
    {
        int count;
        MPI_Get_count(status, type, &count);
        int cancelled;
        MPI_Test_cancelled(status, &cancelled);
        printf("%s = Status(source=%d, tag=%d, err=%d, count=%d, cancelled=%d)\n", name, status->MPI_SOURCE, status->MPI_TAG, status->MPI_ERROR, count, cancelled);
    }
}

int status_equals( MPI_Status* status1, MPI_Status* status2, MPI_Datatype type )
{
    if ( status1->MPI_SOURCE != status2->MPI_SOURCE ) return 0;
    if ( status1->MPI_TAG != status2->MPI_TAG ) return 0;
    if ( status1->MPI_ERROR != status2->MPI_ERROR ) return 0;

    int count1, count2;
    MPI_Get_count(status1, type, &count1);
    MPI_Get_count(status2, type, &count2);
    if ( count1 != count2 ) return 0;

    int cancelled1, cancelled2;
    MPI_Test_cancelled(status1, &cancelled1);
    MPI_Test_cancelled(status2, &cancelled2);
    if ( cancelled1 != cancelled2 ) return 0;

    return 1;
}

int check_status( int test, MPI_Status* status_probed, MPI_Status* status_received)
{
    if ( !status_equals( status_probed, status_received, MPI_INT ) )
    {
        printf("Test %d: Probed status != received status\n", test);
        print_status("status_probed", status_probed, MPI_INT);
        print_status("status_received", status_probed, MPI_INT);
        return 1;
    }
    return 0;
}


int main(int argc, char** argv){

    MTest_Init(&argc, &argv);
    int wrank, wsize;
    MPI_Comm_rank(MPI_COMM_WORLD, &wrank);
    MPI_Comm_size(MPI_COMM_WORLD, &wsize);

    if ( wsize != 2 )
    {
        printf("Exactly 2 MPI processes are needed for this test\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
        return 1;
    }

    int errs = 0;
    // receiver
    if ( wrank == 0 )
    {
        MPI_Status status_probed;
        MPI_Status status_received;
        MPI_Message message;
        MPI_Request request = MPI_REQUEST_NULL;
        int count = 0;
        int recv_buf[MAX_BUF];
        int flag = 0;

        int test = -1;

        // Probe, Recv
        ++test;
        reset_recv_buf(recv_buf);
        MPI_Barrier(MPI_COMM_WORLD);
        MPI_Probe(/*source*/1, /*tag*/test, /*comm*/MPI_COMM_WORLD, &status_probed);
        MPI_Get_count( &status_probed, MPI_INT, &count );
        MPI_Recv(recv_buf, count, MPI_INT, /*source*/1, /*tag*/test, /*comm*/MPI_COMM_WORLD, &status_received);
        errs += check_recv_buf(test, count, recv_buf);
        errs += check_status(test, &status_probed, &status_received);
#if VERBOSE
        printf("Test %d: ", test);
        print_status("status received", &status_received, MPI_INT);
#endif

        // Iprobe..., Recv
        ++test;
        reset_recv_buf(recv_buf);
        MPI_Iprobe(/*source*/1, /*tag*/test, /*comm*/MPI_COMM_WORLD, &flag, MPI_STATUS_IGNORE);
        if ( flag )
        {
            errs++;
            printf("Test %d: Expected unsuccessful Iprobe\n", test);
        }
        MPI_Barrier(MPI_COMM_WORLD);
        do
        {
            MPI_Iprobe(/*source*/1, /*tag*/test, /*comm*/MPI_COMM_WORLD, &flag, &status_probed);
        } while ( !flag );
        MPI_Get_count( &status_probed, MPI_INT, &count );
        MPI_Recv(recv_buf, count, MPI_INT, /*source*/1, /*tag*/test, /*comm*/MPI_COMM_WORLD, &status_received);
        errs += check_recv_buf(test, count, recv_buf);
        errs += check_status(test, &status_probed, &status_received);
#if VERBOSE
        printf("Test %d: ", test);
        print_status("status received", &status_received, MPI_INT);
#endif

        // Probe, Irecv, Wait
        ++test;
        reset_recv_buf(recv_buf);
        MPI_Barrier(MPI_COMM_WORLD);
        MPI_Probe(/*source*/1, /*tag*/test, /*comm*/MPI_COMM_WORLD, &status_probed);
        MPI_Get_count( &status_probed, MPI_INT, &count );
        MPI_Irecv(recv_buf, count, MPI_INT, /*source*/1, /*tag*/test, /*comm*/MPI_COMM_WORLD, &request);
        MPI_Wait(&request, &status_received);
        errs += check_recv_buf(test, count, recv_buf);
        errs += check_status(test, &status_probed, &status_received);
#if VERBOSE
        printf("Test %d: ", test);
        print_status("status received", &status_received, MPI_INT);
#endif

        // Mprobe, Mrecv
        ++test;
        reset_recv_buf(recv_buf);
        MPI_Barrier(MPI_COMM_WORLD);
        MPI_Mprobe(/*source*/1, /*tag*/test, /*comm*/MPI_COMM_WORLD, &message, &status_probed);
        MPI_Get_count( &status_probed, MPI_INT, &count );
        MPI_Mrecv(recv_buf, count, MPI_INT, &message, &status_received);
        errs += check_recv_buf(test, count, recv_buf);
        errs += check_status(test, &status_probed, &status_received);
#if VERBOSE
        printf("Test %d: ", test);
        print_status("status received", &status_received, MPI_INT);
#endif

        // Improbe..., Mrecv
        ++test;
        reset_recv_buf(recv_buf);
        MPI_Improbe(/*source*/1, /*tag*/test, /*comm*/MPI_COMM_WORLD, &flag, &message, MPI_STATUS_IGNORE);
        if ( flag )
        {
            errs++;
            printf("Test %d: Expected unsuccessful Improbe\n", test);
        }
        MPI_Barrier(MPI_COMM_WORLD);
        do
        {
            MPI_Improbe(/*source*/1, /*tag*/test, /*comm*/MPI_COMM_WORLD, &flag, &message, &status_probed);
        } while ( !flag );
        MPI_Get_count( &status_probed, MPI_INT, &count );
        MPI_Mrecv(recv_buf, count, MPI_INT, &message, &status_received);
        errs += check_recv_buf(test, count, recv_buf);
        errs += check_status(test, &status_probed, &status_received);
#if VERBOSE
        printf("Test %d: ", test);
        print_status("status received", &status_received, MPI_INT);
#endif

        // Mprobe, Imrecv, Wait
        ++test;
        reset_recv_buf(recv_buf);
        MPI_Barrier(MPI_COMM_WORLD);
        MPI_Mprobe(/*source*/1, /*tag*/test, /*comm*/MPI_COMM_WORLD, &message, &status_probed);
        MPI_Get_count( &status_probed, MPI_INT, &count );
        MPI_Imrecv(recv_buf, count, MPI_INT, &message, &request);
        MPI_Wait(&request, &status_received);
        errs += check_recv_buf(test, count, recv_buf);
        errs += check_status(test, &status_probed, &status_received);
#if VERBOSE
        printf("Test %d: ", test);
        print_status("status received", &status_received, MPI_INT);
#endif
    }
    // sender
    else
    {
        int send_buf[MAX_BUF];
        for ( int test = 0; test < NUM_TESTS; ++test )
        {
            int n = buffer_elements(test);
            prepare_send_buf(n, send_buf);
            MPI_Barrier(MPI_COMM_WORLD);
            MPI_Send(send_buf, n, MPI_INT, /*dest*/0, /*tag*/test, /*comm*/MPI_COMM_WORLD);
        }
    }
    MTest_Finalize(errs);
    return MTestReturnValue(errs);
}
