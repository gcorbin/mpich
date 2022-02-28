import argparse
import re
import traceback


def parse_filter_output(filter_output):
    discovered_pattern = r'Discovered (\d+) tests in total'
    retained_pattern = r'Retained (\d+) tests'
    discarded_pattern = r'Discarded (\d+) tests'

    discovered = -1
    discarded = -1
    retained = -1

    for line in filter_output:
        m = re.search(discovered_pattern, line)
        if m:
            if discovered >= 0:
                raise RuntimeError("Pattern '{}' matched more than once".format(discovered_pattern))
            discovered = int(m.group(1))
            continue

        m = re.search(retained_pattern, line)
        if m:
            if retained >= 0:
                raise RuntimeError("Pattern '{}' matched more than once".format(retained_pattern))
            retained = int(m.group(1))
            continue

        m = re.search(discarded_pattern, line)
        if m:
            if discarded >= 0:
                raise RuntimeError("Pattern '{}' matched more than once".format(discarded_pattern))
            discarded = int(m.group(1))
            continue

    if discovered < 0:
        raise RuntimeError("Did not find pattern '{}'".format(discovered_pattern))

    if retained < 0:
        raise RuntimeError("Did not find pattern '{}'".format(retained_pattern))

    if discarded < 0:
        raise RuntimeError("Did not find pattern '{}'".format(discarded_pattern))

    return discovered, discarded, retained


def parse_tap_output(build_output):
    total_pattern = r'^1\.\.(\d+)$'

    total = -1

    ok_skipped = 0
    not_ok = 0

    for line in build_output:
        m = re.match(r'^(not\s+)?ok\s+-\s+([^#]*)\s+#(.*)', line)
        if m:
            if m.group(1) is None:
                if re.search(r'SKIP', m.group(3)):
                    ok_skipped += 1
            else:
                not_ok += 1

            continue
        m = re.match(total_pattern, line)
        if m:
            if total >= 0:
                raise RuntimeError("Pattern '{}' matched more than once".format(total_pattern))
            total = int(m.group(1))
            continue

    if total < 0:
        raise RuntimeError("Did not find pattern '{}'".format(total_pattern))

    if (not_ok + ok_skipped ) > total:
        raise RuntimeError("The numbers of skipped({}) + not ok({}) tests "
                           "are more than the reported total of {}".format(ok_skipped, not_ok, total))


    return total, not_ok, ok_skipped, total - not_ok - ok_skipped


def parse_run_output(run_output):

    total_pattern = r'^Tests passed:\s+(\d+);\s+test failed:\s+(\d+)'

    failed = -1
    passed = -1

    failed_lines = 0

    for line in run_output:

        m = re.match(r'^# srun', line)
        if m:
            failed_lines += 1
            continue

        m = re.match(total_pattern, line)
        if m:
            if failed >= 0 or passed >= 0:
                raise RuntimeError("Pattern '{}' matched more than once".format(total_pattern))
            passed = int(m.group(1))
            failed = int(m.group(2))
            continue

    if failed != failed_lines:
        raise RuntimeError("The number of failed tests({}) does not match the reported total of {}".format(failed, failed_lines))

    return (failed + passed), failed, passed



if __name__ == '__main__':
    parser = argparse.ArgumentParser(
        formatter_class=argparse.RawDescriptionHelpFormatter,
        description="Read outputs of all three stages of running the test suite\n"
        "and report on the numbers of passed/failed tests. \n"
        "Return codes: \n"
        " 0 : All tests passed\n"
        " 1 : Some tests failed\n"
        " 2 : Error while parsing arguments to this script\n"
        " 3 : Error while parsing the input files\n"
        " 4 : Other exception occured \n")
    parser.add_argument('filter_output', help="Output from generate_filtered_testlists.py")
    parser.add_argument('build_output', help="Output in .tap format from runtests")
    parser.add_argument('run_output', help="Output from checktests")

    # exit with status 2 on error
    args = parser.parse_args()

    try:
        with open(args.filter_output, 'r') as file:
            filter_num_discovered, filter_num_discarded, filter_num_retained = parse_filter_output(file)

        with open(args.build_output, 'r') as file:
            build_num_total, build_num_failed, build_num_skipped, build_num_built = parse_tap_output(file)
            
        if filter_num_retained != build_num_total:
            raise RuntimeError("Mismatch in reported number of tests:"
                "Filter output ({}) reports {:4d} retained tests, "
                "but build output ({}) reports {:4d} tests in total.".format(
                    args.filter_output,
                    filter_num_retained,
                    args.build_output,
                    build_num_total))

        with open(args.run_output, 'rt', encoding='iso8859_15') as file:
            run_num_total, run_num_failed, run_num_passed = parse_run_output(file)

        if build_num_built != run_num_total:
            raise RuntimeError("Mismatch in reported number of tests:"
                "Build output ({}) reports {:4d} built tests, "
                "but run output ({}) reports {:4d} tests in total.".format(
                    args.build_output,
                    build_num_built,
                    args.run_output,
                    run_num_total))
                
    except RuntimeError as err:
        traceback.print_exc()
        exit(3)
    except Exception as err:
        traceback.print_exc()
        exit(4)


    print("--- Filter stage ({}) ---".format(args.filter_output))
    print("  {:4d} tests discovered".format(filter_num_discovered))
    print("- {:4d} tests discarded".format(filter_num_discarded))
    print("= {:4d} tests retained".format(filter_num_retained))
            
    print("--- Build stage ({}) ---".format(args.build_output))
    print("  {:4d} tests retained".format(build_num_total))
    print("- {:4d} tests skipped".format(build_num_skipped))
    print("- {:4d} tests failed to build".format(build_num_failed))
    print("= {:4d} tests built successfully".format(build_num_built))
    
    print("--- Run stage ({}) ---".format(args.run_output))
    print("  {:4d} tests built successfully".format(run_num_total))
    print("- {:4d} tests failed to run".format(run_num_failed))
    print("= {:4d} tests passed".format(run_num_passed))
    
    print("--- Total ---")
    if build_num_failed > 0 or run_num_failed > 0:
        print("{:4d} tests failed".format(build_num_failed + run_num_failed))
        exit(1)
    
    print("All tests passed")
    exit(0)
