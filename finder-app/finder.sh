#!/bin/bash
# FInder script for AESD assignment 1
# Author: Ravi Agarwal

# Check if both arguments are provided
if [ $# -lt 2 ]; then
	echo "Error: missing argumnets"
	echo "Usage: $0 <filesdir> <searchstr>"
       exit 1
fi


filesdir=$1
searchstr=$2

# check if filesdir is a valid directory 
if [ ! -d "$filesdir" ]; then 
	echo "Error: $filesdir is not a valid directory"
	exit 1
fi

# Count number of files in the directory 
num_files=$(find "$filesdir" -type f | wc -l)

# Count number of macting lines containing searchstr
num_matches=$(grep -r "$searchstr" "$filesdir" 2>/dev/null | wc -l)

# FInal Output 
echo "The number of files are $num_files and the number of matching lines are $num_matches"




