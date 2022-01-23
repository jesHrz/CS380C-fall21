
#!/usr/bin/env bash

mkdir -p build

for PROGRAM in mmm collatz gcd hanoifibfac loop prime \
    regslarge struct sort sieve
do
    ./run-one.sh ${PROGRAM}
done
echo "md5sum hash of outputs"
md5 build/*.txt
