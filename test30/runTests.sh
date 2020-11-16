#!/bin/bash

inputDir=$1
if [ ! -d "$inputDir" ]; then
    echo First Input $inputDir is not an existing dir
    exit
fi

outputDir=$2
if [ ! -d "$outputDir" ]; then
    echo Second Input $outputDir is not an existing dir
    exit
fi

maxThreads=$3
if [[ -n ${maxThreads//[0-9]/} ]]; then
    echo Third Input $maxThreads is not a positive int
    exit
fi

for input in ${inputDir}/*.txt
do  
    for nThread in $(seq 1 $maxThreads)
    do
        inputName=$(basename $input)
        echo InputFile=$inputName NumThreads=$nThread
        outputFile=${outputDir}/${inputName%.txt}-${nThread}.txt
        ./tecnicofs "$input" "$outputFile" $nThread | tail -n 1
    done
done
