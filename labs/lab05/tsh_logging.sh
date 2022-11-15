#!/bin/bash

echo -n > tsh.out

for i in {1..16}
do
    if [[ $i -lt 10 ]]
    then
        make "test0$i" >> tsh.out
    else
        make "test$i" >> tsh.out
    fi
done
