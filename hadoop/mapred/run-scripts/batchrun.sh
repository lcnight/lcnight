#! /bin/bash

mkdir logs

#months='201201 201202 201203 201204 201205'
months='201102 201103 201104 201105 201106 201107 201108 201109 201110 201111 201112 201201 201202 201203 201204 201205'
for m in $months; do
    bash -x ./computeGame.sh $m >>logs/log-$m.txt 2>&1
done
