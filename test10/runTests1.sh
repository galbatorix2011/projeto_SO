#!/bin/bash
inputDir=$1
outputDir=$2
maxThreads=$3
for input in ${inputDir}/*.txt
do  
    for nThread in $(seq 1 $maxThreads)
    do
        inputName=$(basename $input)
        echo InputFile=$inputName NumThreads=$nThread
        outputFile=${outputDir}/${inputName%.txt}-${nThread}.txt
        ./tecnicofs < tecnicofs "$input" "$outputFile" $nThread > aux.txt
        lastLine=$( tail -n 1 aux.txt )
        rm aux.txt
        echo ${lastLine}
    done
done
