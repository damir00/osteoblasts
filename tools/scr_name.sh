#!/bin/bash

if [ -z "$1" ]; then
	echo "$0 filename"
	exit
fi

mv -i "$1" "`date +%d_%m_%y_`$1.png"

