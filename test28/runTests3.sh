#!/bin/bash
inputDir=$1
outputDir=$2
maxThreads=$3
cd ${inputDir}
for input in *.txt
do  
    for nThread in $(seq 1 ${maxThreads})
    do
        echo InputFile=${input} NumThreads=${nThread} 
        cd ..
        outputFile=${outputDir}/${input%.txt}-${nThread}.txt
        ./tecnicofs < tecnicofs "${inputDir}/$input" "$outputFile" $nThread > aux.txt
        lastLine=$( tail -n 1 aux.txt )
        rm aux.txt
        echo ${lastLine}
        cd ${inputDir}
    done
done