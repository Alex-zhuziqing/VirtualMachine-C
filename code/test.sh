#!/bin/bash

for t in tests/*.x2017;
do
	filename=$(echo $t | cut -d '.' -f1)
    ./vm_x2017 $filename.x2017 > $filename.out 2>&1
    diff $filename.out $filename.expected
    if [ $? == 1 ]
        then echo "TESTCASE $filename FAILED"
    else
        echo "TESTCASE $filename SUCCESS"
    fi
done;

