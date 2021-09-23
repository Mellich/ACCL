#!/bin/bash

RESULTS=nop.txt

if [ -f "$RESULTS" ]; then
	rm $RESULTS
fi

for i in {1..50}
do
	./scripts/run1.sh  2>/dev/null | grep t_execute_kernel | awk -F " " '{print $2}' >> $RESULTS
done

awk '{SUM+=$1} END {print SUM/NR}' $RESULTS
