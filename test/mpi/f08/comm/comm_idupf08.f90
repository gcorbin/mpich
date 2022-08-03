!
! Adapted from comm/comm_idup.c to Fortran
!
program main
    use mpi_f08
    implicit none

    integer errs, ierr
    integer rank, wsize
    integer i,k
    type(MPI_Comm) newcomm
    type(MPI_Request) idup_req
    integer name_len
    character(len=MPI_MAX_OBJECT_NAME) :: comm_name, comm_name_get

    real, dimension(10) :: buf, red


    errs = 0
    call mtest_init( ierr )

    call MPI_Comm_rank(MPI_COMM_WORLD, rank, ierr)
    call MPI_Comm_size(MPI_COMM_WORLD, wsize, ierr)

    if (wsize .lt. 2) then
        call MPI_Abort(MPI_COMM_WORLD, 1, ierr)
    end if

    !
    ! Make rank 0 wait in a blocking recv until all other processes
    ! have posted their MPI_Comm_idup ops, then post last.  Should ensure that
    ! idup doesn't block on the non-zero ranks, otherwise we'll get a deadlock.
    if (rank .eq. 0) then
        do i=1,wsize-1
            call MPI_Recv(MPI_BOTTOM, 0, MPI_INTEGER, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE, ierr)
        end do
        call MPI_Comm_idup(MPI_COMM_WORLD, newcomm, idup_req, ierr)
    else
        call MPI_Comm_idup(MPI_COMM_WORLD, newcomm, idup_req, ierr)
        call MPI_Ssend(MPI_BOTTOM, 0, MPI_INTEGER, 0, 0, MPI_COMM_WORLD, ierr)
    end if
    call MPI_Wait(idup_req, MPI_STATUS_IGNORE, ierr)

    ! Set and get the name our communicator
    comm_name = 'Idup of MPI_COMM_WORLD'
    do k=1,len(comm_name_get)
        comm_name_get(k:k) = 'x'
    end do
    call MPI_Comm_set_name(newcomm, comm_name, ierr)
    call MPI_Comm_get_name(newcomm, comm_name_get, name_len, ierr)
    if (comm_name .ne. comm_name_get) then
        write(*,*) 'Error in setting the name of the new communicator'
        errs = errs + 1
    end if

    ! Do some communication to make sure that newcomm works
    red = 0.
    do k=1,size(buf)
        buf(k) = cos(real(k))
    end do
    call MPI_Allreduce(buf, red, size(buf), MPI_REAL, MPI_SUM, newcomm, ierr)

    if (maxval(abs(red - wsize*buf)) > 1e-14 ) then
        write(*, *) 'Error in Allreduce result buffer'
        errs = errs + 1
    end if

    call MPI_Comm_free(newcomm, ierr)

    call mtest_finalize( errs )
end program
