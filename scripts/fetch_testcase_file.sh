#!/bin/bash
noj_testfile_dir=noj/
if [ $# != 2 ]; then
    echo "USAGE: $0 remotefile local"
    echo $#
    exit 50
fi

sftp -i ../conf/id_rsa noj@localhost <<END_SFTP
get $noj_testfile_dir$1 $2
bye
END_SFTP


#Check Download file
if [ -d $2 ]; then
    if [ ! -r $2${1##*/} ]; then
        exit 1;
    fi
else
    if [ ! -r $2 ]; then
        exit 1;
    fi
fi
