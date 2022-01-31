"""
Read a .tap file produced by 'runtests' and extract some information from it.

The raw .tap files can be quite large as they contain almost all output of the tests.
This program generates a report in .json format in which tests are grouped by folder, their success status and possibly
reason for the failure.
The report also lists MPI and MTest calls in the test sources.

Additionally a summary statistic is output to the console, listing the failure rates for each subdirectory.
"""
import json
import re
import argparse
import os

from python_utils.read_sources_from_makefile import read_sources_from_makefile


def is_tap_file(filename):
    name, ext = os.path.splitext(filename)
    return ext == '.tap' and os.path.isfile(filename)


def read_tap_file(filename):
    if not is_tap_file(filename):
        raise FileNotFoundError(f"{filename} is not a valid tapfile.")

    results = []

    parser_state = 'result'

    with open(file=filename, mode='rt', encoding='iso8859_15') as tapfile:
        for line in tapfile:
            # Always look for a result line
            m = re.match(r'^(not\s+)?ok\s+-\s+([^#]*)\s+#', line)
            if m:
                exe_and_nprocs = m.group(2)
                exe_and_nprocs_list = exe_and_nprocs.split()
                exe = exe_and_nprocs_list[0]
                nprocs = exe_and_nprocs_list[1] if len(exe_and_nprocs_list) > 1 else None
                passed = m.group(1) is None
                results.append({
                    'name': exe_and_nprocs,
                    'exe': exe,
                    'nprocs': nprocs,
                    'passed': passed,
                    'timeout': False,
                    'other-errors': [],
                    'scorep-errors': dict()})
                if not passed:
                    parser_state = 'skip to test output'
                    continue

            if parser_state == 'skip to test output':
                m = re.match(r"## Test output \(expected 'No Errors'\):", line)
                if m:
                    parser_state = 'errors'
            elif parser_state == 'errors':
                if re.search(r'APPLICATION TIMED OUT', line):
                    results[-1]['timeout'] = True
                    continue
                m = re.match(r"^## \[Score-P]\s*([^:]+:\d+):(.*)", line)
                if m:
                    results[-1]['scorep-errors'][m.group(1)] = m.group(2)
                    continue
                if len(results[-1]['other-errors']) < 10:
                    results[-1]['other-errors'].append(line)

    return results


def extract_calls(filename):
    call_set = set()
    try:
        with open(filename) as file:
            for line in file:
                for c in re.findall(r"mpi_\w+\b", line, flags=re.IGNORECASE):
                    call_set.add(c.lower())
                for c in re.findall(r"mtest\w+\b", line, flags=re.IGNORECASE):
                    call_set.add(c.lower())
    except FileNotFoundError:
        pass
    return call_set


def group_results_by_folder(results):
    grouped_results = {}
    for item in results:
        dir, exe = os.path.split(item['exe'])
        if dir not in grouped_results:
            grouped_results[dir] = []
        grouped_results[dir].append(item)
    return grouped_results


def analyze_results(result_list):
    results = group_results_by_folder(result_list)

    report = {}
    for folder, tests in results.items():
        makefile = os.path.join(folder, 'Makefile')
        sources = read_sources_from_makefile(makefile)

        relevant_calls_for_exe = {}
        for exe, source_list in sources.items():
            path_to_exe = os.path.join(folder, exe)
            calls = set()
            for source in source_list:
                path_to_source = os.path.join(folder, source)
                calls = calls.union(extract_calls(path_to_source))
            relevant_calls_for_exe[path_to_exe] = calls

        def accumulate_mpi_calls(list_of_exes):
            calls = set()
            for exe in list_of_exes:
                calls_for_exe = relevant_calls_for_exe.get(exe, set())
                calls = calls.union(calls_for_exe)
            return calls

        num_tests = len(tests)
        num_tests_passed = 0
        num_tests_failed = 0
        passed = []
        failed = []
        failed_timeout = []
        failed_scorep = {}
        failed_other = {}

        for test in tests:
            if test['passed']:
                num_tests_passed += 1
                passed.append(test['name'])
                continue
            other_failure = True
            num_tests_failed += 1
            failed.append(test['name'])
            if test['timeout']:
                failed_timeout.append(test['name'])
                other_failure = False
            for scorep_err, scorep_msg in test['scorep-errors'].items():
                if scorep_err not in failed_scorep:
                    failed_scorep[scorep_err] = []
                failed_scorep[scorep_err].append(test['name'])
                other_failure = False
            if other_failure:
                first_failure = ''
                for err in test['other-errors']:
                    m = re.match(r'^##\s+No Errors', err, flags=re.IGNORECASE)
                    if not m:
                        first_failure = err
                        break
                first_failure = first_failure[:min(len(first_failure), 120)]
                first_failure = re.sub(r'\d+', 'XXX', first_failure)
                if first_failure not in failed_other:
                    failed_other[first_failure] = []
                failed_other[first_failure].append(test['name'])

        mpi_call_set = accumulate_mpi_calls([test['exe'] for test in tests])
        never_passed = mpi_call_set.difference(accumulate_mpi_calls([t.split()[0] for t in passed]))
        never_failed = mpi_call_set.difference(accumulate_mpi_calls([t.split()[0] for t in failed]))
        passed_and_failed = mpi_call_set.difference(never_passed.union(never_failed))

        report[folder] = {
            'summary': {
                '#total': num_tests,
                '#passed': num_tests_passed,
                '#failed': num_tests_failed,
            },
            'passed': passed,
            'failed timeout': failed_timeout,
            'failed scorep': failed_scorep,
            'failed other': failed_other,
            'calls': {
                'never passed': sorted(list(never_passed)),
                'never failed': sorted(list(never_failed)),
                'passed and failed': sorted(list(passed_and_failed))
            }
        }

    # Output stats for each folder
    all_tests = 0
    all_failed = 0
    all_passed = 0
    for folder, entry in report.items():
        failed = entry['summary']['#failed']
        passed = entry['summary']['#passed']
        total = entry['summary']['#total']
        percent_failed = (100 * failed) // total

        all_tests += total
        all_failed += failed
        all_passed += passed

        print(
            f"{folder:20s}: {failed:4d} of {total:4d} tests failed ({percent_failed:3d}%). "
            f"[{'|' * (percent_failed // 5):20s}]"
        )

    # Print a grand total
    percent_all_failed = (100 * all_failed) // all_tests
    print("-" * 80)
    print(
        f"{'total':20s}: {all_failed:4d} of {all_tests:4d} tests failed ({percent_all_failed:3d}%)."
        f" [{'|' * (percent_all_failed // 5):20s}]"
    )

    return report


def write_json(obj, out_file):
    with open(out_file, 'w') as outf:
        json.dump(obj, outf, indent=4)


def read_result_list(in_file):
    with open(in_file) as inf:
        result_list = json.load(inf)
    return result_list


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('input')
    parser.add_argument('output')
    flags = parser.add_mutually_exclusive_group()
    flags.add_argument('--stop-at-intermediate', '-si', action='store_true')
    flags.add_argument('--continue-from-intermediate', '-ci', action='store_true')

    args = parser.parse_args()

    input_name, input_ext = os.path.splitext(args.input)
    output_name, output_ext = os.path.splitext(args.output)
    assert output_ext == '.json'

    result_list = []
    if args.continue_from_intermediate:
        assert input_ext == '.json'
        result_list = read_result_list(args.input)
    else:
        assert input_ext == '.tap'
        result_list = read_tap_file(args.input)

    if args.stop_at_intermediate:
        write_json(result_list, args.output)
    else:
        report = analyze_results(result_list)
        write_json(report, args.output)
