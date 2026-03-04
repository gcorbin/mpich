program all_probe_sequences

use :: mpi_f08

implicit none

integer, parameter :: BUF_SCALE = 10
integer, parameter :: NUM_TESTS = 6
logical, parameter :: verbose = .false.

integer :: wrank, wsize
integer :: errs, ierr
integer :: test

integer :: buffer(BUF_SCALE*NUM_TESTS)

type(MPI_Status) :: status_probed, status_received
type(MPI_Message) :: message
type(MPI_Request) :: request
integer :: nelem
logical :: flag


call MTest_Init( ierr )
call MPI_Comm_rank(MPI_COMM_WORLD, wrank, ierr)
call MPI_Comm_size(MPI_COMM_WORLD, wsize, ierr)

if ( wsize .lt. 2 ) then
    print *, "At least 2 MPI processes are needed for this test"
    call MPI_Abort(MPI_COMM_WORLD, 1, ierr)
end if

errs = 0
! Receiver
if ( wrank .eq. 0 ) then
    ! Probe, Recv
    test = 0
    call reset_recv_buf(buffer)
    call reset_status_public_fields(status_probed)
    call reset_status_public_fields(status_received)
    call MPI_Probe(1, test, MPI_COMM_WORLD, status_probed, ierr)
    call MPI_Get_count(status_probed, MPI_INTEGER, nelem, ierr)
    call MPI_Recv(buffer, nelem, MPI_INTEGER, 1, test, MPI_COMM_WORLD, status_received, ierr)
    call check_return_value("MPI_Recv", ierr)
    call check_recv_buf(test, nelem, buffer)
    call check_status(test, status_probed, status_received)
    if ( verbose ) then
        print '("Test ", i0)', test
        call print_status("status_received", status_received, MPI_INTEGER)
    end if

    ! Iprobe..., Recv
    test = test + 1
    call reset_recv_buf(buffer)
    call reset_status_public_fields(status_probed)
    call reset_status_public_fields(status_received)
    flag = .false.
    do while ( .not. flag )
        call MPI_Iprobe(1, test, MPI_COMM_WORLD, flag, status_probed, ierr)
    end do
    call MPI_Get_count(status_probed, MPI_INTEGER, nelem, ierr)
    call MPI_Recv(buffer, nelem, MPI_INTEGER, 1, test, MPI_COMM_WORLD, status_received, ierr)
    call check_return_value("MPI_Recv", ierr)
    call check_recv_buf(test, nelem, buffer)
    call check_status(test, status_probed, status_received)
    if ( verbose ) then
        print '("Test ", i0)', test
        call print_status("status_received", status_received, MPI_INTEGER)
    end if

    ! Probe, Irecv, Wait
    test = test + 1
    call reset_recv_buf(buffer)
    call reset_status_public_fields(status_probed)
    call reset_status_public_fields(status_received)
    call MPI_Probe(1, test, MPI_COMM_WORLD, status_probed, ierr)
    call MPI_Get_count(status_probed, MPI_INTEGER, nelem, ierr)
    call MPI_Irecv(buffer, nelem, MPI_INTEGER, 1, test, MPI_COMM_WORLD, request, ierr)
    call MPI_Wait(request, status_received, ierr)
    call check_return_value("MPI_Wait", ierr)
    call check_recv_buf(test, nelem, buffer)
    call check_status(test, status_probed, status_received)
    if ( verbose ) then
        print '("Test ", i0)', test
        call print_status("status_received", status_received, MPI_INTEGER)
    end if

    ! Mprobe, Mrecv
    test = test + 1
    call reset_recv_buf(buffer)
    call reset_status_public_fields(status_probed)
    call reset_status_public_fields(status_received)
    call MPI_Mprobe(1, test, MPI_COMM_WORLD, message, status_probed, ierr)
    call MPI_Get_count(status_probed, MPI_INTEGER, nelem, ierr)
    call MPI_Mrecv(buffer, nelem, MPI_INTEGER, message, status_received, ierr)
    call check_return_value("MPI_Mrecv", ierr)
    call check_recv_buf(test, nelem, buffer)
    call check_status(test, status_probed, status_received)
    if ( verbose ) then
        print '("Test ", i0)', test
        call print_status("status_received", status_received, MPI_INTEGER)
    end if

    ! Improbe..., Mrecv
    test = test + 1
    call reset_recv_buf(buffer)
    call reset_status_public_fields(status_probed)
    call reset_status_public_fields(status_received)
    flag = .false.
    do while(.not. flag)
        call MPI_Improbe(1, test, MPI_COMM_WORLD, flag, message, status_probed, ierr)
    end do
    call MPI_Get_count(status_probed, MPI_INTEGER, nelem, ierr)
    call MPI_Mrecv(buffer, nelem, MPI_INTEGER, message, status_received, ierr)
    call check_return_value("MPI_Mrecv", ierr)
    call check_recv_buf(test, nelem, buffer)
    call check_status(test, status_probed, status_received)
    if ( verbose ) then
        print '("Test ", i0)', test
        call print_status("status_received", status_received, MPI_INTEGER)
    end if

    ! Mprobe, Imrecv, Wait
    test = test + 1
    call reset_recv_buf(buffer)
    call reset_status_public_fields(status_probed)
    call reset_status_public_fields(status_received)
    call MPI_Mprobe(1, test, MPI_COMM_WORLD, message, status_probed, ierr)
    call MPI_Get_count(status_probed, MPI_INTEGER, nelem, ierr)
    call MPI_Imrecv(buffer, nelem, MPI_INTEGER, message, request, ierr)
    call MPI_Wait(request, status_received, ierr)
    call check_return_value("MPI_Wait", ierr)
    call check_recv_buf(test, nelem, buffer)
    call check_status(test, status_probed, status_received)
    if ( verbose ) then
        print '("Test ", i0)', test
        call print_status("status_received", status_received, MPI_INTEGER)
    end if

! Sender
else if ( wrank .eq. 1 ) then
    do test = 0, NUM_TESTS-1
        call prepare_send_buf(buffer_elements(test), buffer)
        call MPI_Send(buffer, buffer_elements(test), MPI_INTEGER, 0, test, MPI_COMM_WORLD, ierr)
    end do
end if

call MTest_Finalize( errs )

contains


subroutine check_return_value(name, return_value)
    character(len=*), intent(in) :: name
    integer, intent(in) :: return_value

    integer :: ierr, result_len
    character(len=MPI_MAX_ERROR_STRING) :: err_string

    if ( return_value .ne. MPI_SUCCESS ) then
        errs = errs + 1
        call MPI_Error_string(return_value, err_string, result_len, ierr)
        print '(a, " returned error ", i0, " : ", a)', name, return_value, err_string
    end if
end subroutine

integer function buffer_elements(test)
    integer, intent(in) :: test
    buffer_elements = ( test + 1 ) * BUF_SCALE
end function

subroutine prepare_send_buf(num, buf)
    integer, intent(in) :: num
    integer, intent(inout) :: buf(:)

    buf(1:num) = 1
    if ( num .lt. size(buf) ) buf(num+1:size(buf)) = 0
end subroutine

subroutine reset_recv_buf(buf)
    integer, intent(inout) :: buf(:)
    buf(:size(buf)) = 0
end subroutine

subroutine check_recv_buf(test, nelem, buf)
    integer, intent(in) :: test, nelem
    integer, intent(in) :: buf(:)

    if ( nelem .ne. buffer_elements(test) ) then
        print '("Test ", i2, "Expected to receive ", i4, " elements, but got ", i4)', test, buffer_elements(test), nelem
        errs = errs + 1
        return
    end if

    if ( any(buf(:nelem) .ne. 1 ) ) then
        print '("Test ", i2, "Recv buffer elements not correct")', test
        errs = errs + 1
        return
    end if
end subroutine

subroutine print_status( name, status, datatype )
    character(len=*), intent(in) :: name
    type(MPI_Status), intent(in) :: status
    type(MPI_Datatype), intent(in) :: datatype

    integer :: nelem, ierr
    logical :: cancelled

    call MPI_Get_count(status, datatype, nelem, ierr)
    call MPI_Test_cancelled(status, cancelled, ierr)

    print '(a, " = Status(source=", i0, ", tag=", i0, ", err=", i0, ", count=", i0, ", cancelled=", b0, ")" )', &
        name, &
        status%MPI_SOURCE, &
        status%MPI_TAG, &
        status%MPI_ERROR, &
        nelem, &
        cancelled
end subroutine

logical function status_equals( status1, status2, datatype ) result(equals)
    type(MPI_Status), intent(in) :: status1, status2
    type(MPI_Datatype), intent(in) :: datatype

    integer :: count1, count2, ierr
    logical :: cancelled1, cancelled2

    equals = &
        status1%MPI_SOURCE .eq. status2%MPI_SOURCE &
        .and. status1%MPI_TAG .eq. status2%MPI_TAG
    if ( .not. equals ) return

    ! We do not check the MPI_ERROR field, because OpenMPI sets this to
    ! some random value
    ! .and. status1%MPI_ERROR .eq. status2%MPI_ERROR

    call MPI_Get_count(status1, datatype, count1, ierr)
    call MPI_Get_count(status2, datatype, count2, ierr)
    equals = count1 .eq. count2
    if ( .not. equals ) return

    call MPI_Test_cancelled(status1, cancelled1, ierr)
    call MPI_Test_cancelled(status2, cancelled2, ierr)
    equals = cancelled1 .eqv. cancelled2
    return
end function

subroutine check_status(test, status_probed, status_received)
    integer, intent(in) :: test
    type(MPI_Status), intent(in) :: status_probed, status_received

    if ( .not. status_equals( status_probed, status_received, MPI_INTEGER) ) then
        errs = errs + 1
        print '("Test ", i2, " Probed status != received status")', test
        call print_status("status_probed", status_probed, MPI_INTEGER)
        call print_status("status_received" ,status_received, MPI_INTEGER)
    end if
end subroutine

subroutine reset_status_public_fields(status)
    type(MPI_Status), intent(inout) :: status

    status%MPI_SOURCE = MPI_PROC_NULL
    status%MPI_TAG = -1
    status%MPI_ERROR = MPI_SUCCESS
end subroutine
end program
