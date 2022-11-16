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

    local str="xxx"
    if [ -f "$file_name" ]; then
        str=$(cube_dump -z incl -c name=/main/ -m "${metric_name}" "${file_name}" | grep "main" | sed "s/main(id=[[:digit:]]\+)//")
        str=$(cleanup_spaces "$str")
    fi
    echo "$str"
}

# Find all test executables in this folder that start with 'ic_bytes_'
# or 'nbic_bytes'
test_names=$(find . -regex '\./\(nb\)?ic_bytes_\w*')
ntests=$(echo "$test_names" | wc -l)
echo "Found $ntests tests"


export SCOREP_EXPERIMENT_DIRECTORY="scorep-latest"
# Enable output in the tests
export MPITEST_VERBOSE=1
tests_passed=0
tests_failed=0

for test_name in $test_names; do
    echo "Test $test_name"

    errs=0
    # Cleanup previous experiment archive, if present
    if [ -d "scorep-latest" ]; then
        rm -r "scorep-latest"
    fi
    # Run the instrumented test
    mpirun -n 6 ${test_name} > test.log

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
        tests_passed=$((tests_passed+1))
    else
        echo "not ok: $errs errors"
        tests_failed=$((tests_failed+1))
    fi
done

echo "------------------------------------------------"
echo "summary:"
echo "  tests passed: $tests_passed"
echo "  tests failed: $tests_failed"
echo ""

if [ $tests_failed -gt 0 ]; then
    exit 1
else
    exit 0
fi
