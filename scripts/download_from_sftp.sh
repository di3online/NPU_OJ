#!/bin/bash
if [ $# != 6 ]; then
    echo "USAGE: $0 remotefile local user host port id_file"
    exit 50
fi

if [ ! -r $6 ]; then
    exit 2
fi

sftp -i $6 -P $5 -o StrictHostKeyChecking=no $3@$4 <<END_SFTP
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

exit 0
