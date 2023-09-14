set -ex
export LAST_MERGE=$( git log --merges -1 --format='%H' )
export COMMIT_AFTER_LAST_MERGE="$( git log '^'${LAST_MERGE} HEAD --format='%H' --max-count=6 | tail -1 )"
# Use clang++, because it is a bit faster and uses less memory than g++
git rebase --exec "echo Running test-one-commit on \$( git log -1 ) && ./autogen.sh && CC=clang CXX=clang++ ./configure && make clean && make -j $(nproc) check && ./test/functional/test_runner.py -j $(( $(nproc) * 2 ))" "${COMMIT_AFTER_LAST_MERGE}~1"
