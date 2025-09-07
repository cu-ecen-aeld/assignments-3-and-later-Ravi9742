#!/bin/bash
# Writer script for AESD Assignment 1
# Author: Ravi Agarwal

# Check if both arguments are provided
if [ $# -lt 2 ]; then
	echo "Error: missing argumnets"
	echo "Usage: $0 <full_path_to_file> <text_to_write>"
       exit 1
fi

writefile=$1
writestr=$2

# Extract the directory path from the full file path
dirpath=$(dirname "$writefile")

# Create the directory path if it doesn't exist
if [ ! -d "$dirpath" ]; then
    mkdir -p "$dirpath"
    if [ $? -ne 0 ]; then
        echo "Error: Could not create directory path '$dirpath'."
        exit 1
    fi
fi

# Write the text to the file (overwriting any existing file)
echo "$writestr" > "$writefile"
if [ $? -ne 0 ]; then
    echo "Error: Could not create or write to file '$writefile'."
    exit 1
fi

# Success message
echo "File '$writefile' created successfully with the given content."

