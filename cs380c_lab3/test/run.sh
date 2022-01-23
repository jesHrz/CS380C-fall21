
#!/usr/bin/env bash

mkdir -p 3addr
mkdir -p build

for PROGRAM in collatz gcd hanoifibfac loop mmm prime \
    regslarge struct sort sieve
do
    ./run-one.sh ${PROGRAM}
done
echo "md5sum hash of outputs"
md5 build/*.txt
