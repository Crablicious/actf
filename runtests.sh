#!/usr/bin/env bash

set -o pipefail

usage()
{
    printf "Usage: %s [-erh]\n" $0
    printf " -e <executable>  Set the testing file name (default a.out)\n"
    printf " -r               Replace refs with current output\n"
    printf " -h               Display this help\n"
}

use_color=1
if [ "$TERM" == "dumb" ]; then # emacs is dumb
    use_color=0
fi

replaceflag=
executable="./a.out"
while getopts rhe: option
do
    case $option in
        e) executable="$OPTARG";;
        r) replaceflag=1;;
        h) usage
           exit 0;;
        ?) usage
           exit 1;;
    esac
done

printnok()
{
    if [ $use_color -eq 0 ]; then
        printf "  ... ${dir} NOK\n"
    else
        printf "  ... ${dir} \033[31mNOK\033[0m\n"
    fi
}

printok()
{
    if [ $use_color -eq 0 ]; then
        printf "  ... ${dir} OK\n"
    else
        printf "  ... ${dir} \033[32mOK\033[0m\n"
    fi
}

test_status=0
n_ok=0
n_nok=0
asan_exitcode=100
for dir in testdata/ctfs/*/; do
    dir=${dir%*/}
    mkdir -p ${dir}/refs
    printf "Test: $dir ...\n"

    if [ "${dir: -4}" != "_nok" ]; then
        want_success=true
    else
        want_success=
    fi

    ASAN_OPTIONS="exitcode=${asan_exitcode}" ${executable} -l ${dir} >${dir}/refs/out.log 2>${dir}/refs/err.log
    status=$?

    if [ ! -z "$replaceflag" ]; then
        echo "Replacing ${dir}/refs/out.ref with ${dir}/refs/out.log"
        cp -f ${dir}/refs/out.log ${dir}/refs/out.ref
    fi

    if [ $status -eq $asan_exitcode ]; then
        cat ${dir}/refs/err.log
        echo "${executable} -l ${dir} has ASAN errors"
        n_nok=$(($n_nok + 1))
        printnok
        continue
    elif [ $want_success ] && [ $status -ne 0 ]; then
        cat ${dir}/refs/err.log
        echo "${executable} -l ${dir} returned ${status}; want 0"
        n_nok=$(($n_nok + 1))
        printnok
        continue
    elif [ ! $want_success ] && [ $status -eq 0 ]; then
        echo "${executable} -l ${dir} returned 0; want an error"
        n_nok=$(($n_nok + 1))
        printnok
        continue
    fi

    diff --text ${dir}/refs/out.ref ${dir}/refs/out.log > ${dir}/refs/out.diff
    if [ $? -ne 0 ]; then
        echo "${executable} -l ${dir} differs from reference: ${dir}/refs/out.diff"
        n_nok=$(($n_nok + 1))
        printnok
        continue
    else
        n_ok=$(($n_ok + 1))
    fi

    printok
done

n_tot=$(($n_nok + $n_ok))
echo "Passed/Total: [${n_ok}/${n_tot}]"
if [ $n_nok -ne 0 ]; then
    exit 1
else
    exit 0
fi
