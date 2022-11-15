#!/bin/bash

# Remove leading and trailing spaces
# Replace multiple spaces by a single space
cleanup_spaces()
{
    local string=$1
    echo "$1" | sed "s/^[[:space:]]\+//" | sed "s/[[:space:]]\+\$//" | sed "s/[[:space:]]\+/ /g"
}

get_cube_metric()
{
    local file_name=$1
    local metric_name=$2

    # Dump the metric $metric_name for callpath 'main' inclusive for all ranks
    # Look for the output line that contains the metrics and keep only
    # the numbers
    local str=$(cube_dump -z incl -c name=/main/ -m "${metric_name}" "${file_name}" | grep "main" | sed "s/main(id=[[:digit:]]\+)//")
    str=$(cleanup_spaces "$str")
    echo "$str"
}

test_names="bcast gather gatherv reduce scatter scatterv"


export SCOREP_EXPERIMENT_DIRECTORY="scorep-latest"
# Enable output in the tests
export MPITEST_VERBOSE=1
errs=0

for test_name in $test_names; do
    echo "Test $test_name"

    # Cleanup previous experiment archive, if present
    if [ -d "scorep-latest" ]; then
        rm -r "scorep-latest"
    fi
    # Run the instrumented test
    mpirun -n 6 ./ic_bytes_${test_name} > test.log

    # Get the expected numbers of bytes from the test output
    sendbytes_expected=$(grep "sendbytes:" test.log | sed "s/sendbytes://")
    sendbytes_expected=$(cleanup_spaces "$sendbytes_expected")
    recvbytes_expected=$(grep "recvbytes:" test.log | sed "s/recvbytes://")
    recvbytes_expected=$(cleanup_spaces "$recvbytes_expected")

    # Get the actual numbers of bytes from the experiment archive
    # profile.cubex
    sendbytes_actual=$(get_cube_metric "scorep-latest/profile.cubex" "bytes_sent")
    recvbytes_actual=$(get_cube_metric "scorep-latest/profile.cubex" "bytes_received")

    # Compare actual and expected
    if [ "$sendbytes_actual" != "$sendbytes_expected" ]; then
        echo "Mismatch in sendbytes:"
        echo "actual: $sendbytes_actual"
        echo "expected: $sendbytes_expected"
        errs=$((errs+1))
    fi

    if [ "$recvbytes_actual" != "$recvbytes_expected" ]; then
        echo "Mismatch in recvbytes:"
        echo "actual  : $recvbytes_actual"
        echo "expected: $recvbytes_expected"
        errs=$((errs+1))
    fi

    if [ $errs -eq 0 ]; then
        echo "ok"
    fi
done

if [ $errs -gt 0 ]; then
    exit 1
else
    exit 0
fi
