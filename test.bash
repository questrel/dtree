#!/bin/bash -x
# run this script in the same directory as your makefile!
set -x
make -j5
if [[ ! -s sql.dat.99 ]] ; then 
   make canonical.out
   rm *.map # shared map not working
   head -99 /usr/share/dict/words | ./canonical.out -S | ./sql.test.out
   cp -av sql.dat sql.dat.99
fi
cp -av  sql.dat.99 sql.dat # restore default
rm *.map # shared map not working
(
#    echo 'ALIAS word.ASCII_word=column(1),word.dictionary_word=column(4), word.letter_product=column(9), word.letter_sum=column(10);' ;
    ./canonical.out -Sv < /dev/null;
echo 'select * from word;
select ASCII_word,NUMERIC_TO_STRING(letter_product,letter_sum) from word where letter_product*letter_sum <= letter_product+letter_sum;' ; cat ) | ./sql.test.out 

