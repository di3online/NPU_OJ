#!/bin/bash
if [ $# != 5 ]; then
    echo "USAGE: $0 remotefile local user host id_file"
    echo $#
    exit 50
fi

if [ ! -r $5 ]; then
    exit 2
fi

sftp -i $5 $3@$host <<END_SFTP
get $1 $2
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
