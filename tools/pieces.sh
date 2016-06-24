#!/bin/bash

max_w=$1
shift

cur_x=0
cur_y=0
cur_max_y=0

map_w=0
map_h=0

cmd=( )

OUT_JSON="out.json"

echo "[" > $OUT_JSON

first=true

for i in $@; do

	p=`convert "$i" -trim info:-`
	size=`echo "$p" | cut -d ' ' -f 3 | tr 'x' ' '`
	pos=`echo "$p" | cut -d ' ' -f 4`
	pos=`echo ${pos#*+} | tr '+' ' '`

	size_x=`echo "$size" | cut -d ' ' -f 1`
	size_y=`echo "$size" | cut -d ' ' -f 2`
	pos_x=`echo "$pos" | cut -d ' ' -f 1`
	pos_y=`echo "$pos" | cut -d ' ' -f 2`

	if [[ $cur_x+$size_x -gt $max_w ]]; then
#		echo "new line map size $map_w,$map_h"
		cur_x=$size_x
		cur_y=$((cur_max_y))

		map_x=0
		map_y=$cur_y
	else
		map_x=$cur_x
		map_y=$cur_y
		cur_x=$((cur_x+size_x))
	fi

	new_y=$((cur_y+size_y))
	cur_max_y=$((new_y>cur_max_y?new_y:cur_max_y))

	map_w=$((cur_x>map_w?cur_x:map_w))
	map_h=$((cur_max_y>map_h?cur_max_y:map_h))

	echo "size $size_x,$size_y, pos $pos_x,$pos_y, map $map_x,$map_y"

	cmd+=( \( $i -trim \) -geometry +${map_x}+${map_y} -composite )

	$first && {
		first=false
	} || {
		echo ",">>$OUT_JSON
	}
	echo -n "{\"x\":$pos_x,\"y\":$pos_y,\"w\":$size_x,\"h\":$size_y,\"map_x\":$map_x,\"map_y\":$map_y}">>$OUT_JSON

done

echo "map size $map_w,$map_h"

echo "]" >> $OUT_JSON

convert -size ${map_w}x${map_h} xc:transparent ${cmd[@]} out.png


