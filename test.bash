#!/bin/bash -x
# run this script in the same directory as your makefile!
make
if [[ ! -e sql.dat.99 ]] ; then 
   head -99 /usr/share/dict/words | canonical.out -S | ./sql.test.out
   cp -av sql.dat sql.dat.99
fi
cp -av  sql.dat.99 sql.dat # restore default
rm *.map # shared map not working
(echo 'select column(1),NUMERIC_TO_STRING(column(9),column(10)) from word where column(9)*column(10) <= column(9)+ column(10);' ; cat ) | ./sql.test.out
