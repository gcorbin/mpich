#!/bin/bash

# Execute this script from the main directory of mpi test, i.e. test/mpi/

wdir=`pwd`/scorep/compiler-wrappers

SCOREP_WRAPPER=off \
./configure \
CC=$wdir/scorep-mpi-only-gcc \
CXX=$wdir/scorep-mpi-only-g++ \
FC=$wdir/scorep-mpi-only-gfortran \
F77=$wdir/scorep-mpi-only-gfortran \
MPICC=$wdir/scorep-mpi-only-mpicc \
MPICXX=$wdir/scorep-mpi-only-mpicxx \
MPIFC=$wdir/scorep-mpi-only-mpif90 \
MPIF77=$wdir/scorep-mpi-only-mpif77 \
--prefix=`pwd`/install \
--disable-dependency-tracking \
--enable-fortran=f77,f90,f08 \
--disable-cxx \
--enable-threads=funneled \
--enable-strictmpi
