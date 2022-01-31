!
! Copyright (C) by Argonne National Laboratory
!     See COPYRIGHT in top-level directory
!

! This file created from f77/util/mtestf.f with f77tof90

        subroutine MTest_Init( ierr )
!       Place the include first so that we can automatically create a
!       Fortran 90 version that uses the mpi module instead.  If
!       the module is in a different place, the compiler can complain
!       about out-of-order statements
        use mpi
        integer ierr
        logical flag
        logical dbgflag
        integer wrank
        common /mtest/ dbgflag, wrank

        call MPI_Initialized( flag, ierr )
        if (.not. flag) then
           call MPI_Init( ierr )
        endif

        dbgflag = .false.
        call MPI_Comm_rank( MPI_COMM_WORLD, wrank, ierr )
        end
!
        subroutine MTest_Finalize( errs )
        use mpi
        integer errs
        integer rank, toterrs, ierr
        
        call MPI_Comm_rank( MPI_COMM_WORLD, rank, ierr )

        call MPI_Allreduce( errs, toterrs, 1, MPI_INTEGER, MPI_SUM,  &
      &        MPI_COMM_WORLD, ierr ) 
        
        if (rank .eq. 0) then
           if (toterrs .gt. 0) then 
                print *, " Found ", toterrs, " errors"
           else
                print *, " No Errors"
           endif
        endif

        call MPI_Finalize( ierr )
        end
!
! A simple get intracomm for now
        logical function MTestGetIntracomm( comm, min_size, qsmaller )
        use mpi
        integer ierr
        integer comm, min_size, size, rank
        logical qsmaller
        integer myindex
        save myindex
        data myindex /0/

        comm = MPI_COMM_NULL
        if (myindex .eq. 0) then
           comm = MPI_COMM_WORLD
        else if (myindex .eq. 1) then
           call mpi_comm_dup( MPI_COMM_WORLD, comm, ierr )
        else if (myindex .eq. 2) then
           call mpi_comm_size( MPI_COMM_WORLD, size, ierr )
           call mpi_comm_rank( MPI_COMM_WORLD, rank, ierr )
           call mpi_comm_split( MPI_COMM_WORLD, 0, size - rank, comm,  &
      &                                 ierr )
        else
           if (min_size .eq. 1 .and. myindex .eq. 3) then
              comm = MPI_COMM_SELF
           endif
        endif
        myindex = mod( myindex, 4 ) + 1
        MTestGetIntracomm = comm .ne. MPI_COMM_NULL
        end
!
        subroutine MTestFreeComm( comm )
        use mpi
        integer comm, ierr
        if (comm .ne. MPI_COMM_WORLD .and. &
      &      comm .ne. MPI_COMM_SELF  .and. &
      &      comm .ne. MPI_COMM_NULL) then
           call mpi_comm_free( comm, ierr )
        endif
        end
!
        subroutine MTestPrintError( errcode )
        use mpi
        integer errcode
        integer errclass, slen, ierr
        character*(MPI_MAX_ERROR_STRING) string

        call MPI_Error_class( errcode, errclass, ierr )
        call MPI_Error_string( errcode, string, slen, ierr )
        print *, "Error class ", errclass, "(", string(1:slen), ")"
        end
!
        subroutine MTestPrintErrorMsg( msg, errcode )
        use mpi
        character*(*) msg
        integer errcode
        integer errclass, slen, ierr
        character*(MPI_MAX_ERROR_STRING) string

        call MPI_Error_class( errcode, errclass, ierr )
        call MPI_Error_string( errcode, string, slen, ierr )
        print *, msg, ": Error class ", errclass, " &
      &       (", string(1:slen), ")" 
        end

        subroutine MTestSpawnPossible( can_spawn, errs )
        use mpi
        integer can_spawn
        integer errs
        integer(kind=MPI_ADDRESS_KIND) val
        integer ierror
        logical flag
        integer comm_size

        call mpi_comm_get_attr( MPI_COMM_WORLD, MPI_UNIVERSE_SIZE, val, &
      &                          flag, ierror )
        if ( ierror .ne. MPI_SUCCESS ) then
!       MPI_UNIVERSE_SIZE keyval missing from MPI_COMM_WORLD attributes
            can_spawn = -1
            errs = errs + 1
        else
            if ( flag ) then
                comm_size = -1

                call mpi_comm_size( MPI_COMM_WORLD, comm_size, ierror )
                if ( ierror .ne. MPI_SUCCESS ) then
!       MPI_COMM_SIZE failed for MPI_COMM_WORLD
                    can_spawn = -1
                    errs = errs + 1
                    return
                endif

                if ( val .le. comm_size ) then
!       no additional processes can be spawned
                    can_spawn = 0
                else
                    can_spawn = 1
                endif
            else
!       No attribute associated with key MPI_UNIVERSE_SIZE of MPI_COMM_WORLD
                can_spawn = -1
            endif
        endif
        end

!       Mark the BEGINNING of a region in the trace file which is ignored in comparisons.
!       The routine itself does nothing, but corresponding events are written into the
!       trace, which serve as instructions to the filter-otf2-print.pl script
        subroutine MTestBeginExcludeFromTrace()
        end subroutine

!       Mark the END of a region in the trace file which is ignored in comparisons.
!       The routine itself does nothing, but corresponding events are written into the
!       trace, which serve as instructions to the filter-otf2-print.pl script
        subroutine MTestEndExcludeFromTrace()
        end subroutine
