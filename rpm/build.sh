#!/bin/bash

##for check
if [ $# -ne 3 ] && [ $# -ne 4 ];then
	echo "Usage: ./build.sh specfilename version buildno [prefix]"
	exit
fi

work_dir=`dirname $0`

if [ $# -eq 4 ];then
	$work_dir/rpm_create -p $4 -v $2 -r $3 $1 -k
else
	$work_dir/rpm_create -p /home/sina -v $2 -r $3 $1 -k
fi
