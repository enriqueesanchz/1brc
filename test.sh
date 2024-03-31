#!/bin/bash

files=("./main.out"
       "./4.out")

for program in "${files[@]}"; do
acum=0
printf "[ "
for n in {1..2}; do
    time=$(echo "$(/usr/bin/time -f '%e' ${program} head.txt 2>&1 >/dev/null)" | bc)
    acum=$(echo "${acum} + ${time}" | bc)
    printf "${time} "
    sleep 1
done

mean=$(echo "scale=2; ${acum}/2" | bc)
printf "] media: ${mean}\n"
done
