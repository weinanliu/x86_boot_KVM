#/bin/sh
PAD=$(size --target=binary -d -A  $1 | grep Total | awk '{print $2 %4096==0?0:4096- $2 %4096}')
if test "x$PAD" = "x0" ; then
    exit
fi
printf '1%.0s' $(seq 1 $PAD) >> $1
