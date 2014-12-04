#!/bin/bash/

file="linear-error"
echo "$file: Mean absolute error"
cat $file.txt | grep "Mean absolute error" | awk 'NR % 2 == 1' | sed 's/.*  //'
echo "$file: Relative absolute error"
cat $file.txt | grep "Relative absolute error" | awk 'NR % 2 == 1' | sed 's/.*  //'
echo "$file: Build Time"
cat $file.txt | grep "Time taken to build model" | sed 's/.*: //' | sed 's/ seconds//'
echo "$file: Validation Time"
cat $file.txt | grep "Time taken to test model on training data" | sed 's/.*: //' | sed 's/ seconds//'

file="m5p-error"
echo "$file: Mean absolute error"
cat $file.txt | grep "Mean absolute error" | awk 'NR % 2 == 1' | sed 's/.*  //'
echo "$file: Relative absolute error"
cat $file.txt | grep "Relative absolute error" | awk 'NR % 2 == 1' | sed 's/.*  //'
echo "$file: Build Time"
cat $file.txt | grep "Time taken to build model" | sed 's/.*: //' | sed 's/ seconds//'
echo "$file: Validation Time"
cat $file.txt | grep "Time taken to test model on training data" | sed 's/.*: //' | sed 's/ seconds//'

file="svm-error"
echo "$file: Mean absolute error"
cat $file.txt | grep "Mean absolute error" | awk 'NR % 2 == 1' | sed 's/.*  //'
echo "$file: Relative absolute error"
cat $file.txt | grep "Relative absolute error" | awk 'NR % 2 == 1' | sed 's/.*  //'
echo "$file: Build Time"
cat $file.txt | grep "Time taken to build model" | sed 's/.*: //' | sed 's/ seconds//'
echo "$file: Validation Time"
cat $file.txt | grep "Time taken to test model on training data" | sed 's/.*: //' | sed 's/ seconds//'

file="svm2-error"
echo "$file: Mean absolute error"
cat $file.txt | grep "Mean absolute error" | awk 'NR % 2 == 1' | sed 's/.*  //'
echo "$file: Relative absolute error"
cat $file.txt | grep "Relative absolute error" | awk 'NR % 2 == 1' | sed 's/.*  //'
echo "$file: Build Time"
cat $file.txt | grep "Time taken to build model" | sed 's/.*: //' | sed 's/ seconds//'
echo "$file: Validation Time"
cat $file.txt | grep "Time taken to test model on training data" | sed 's/.*: //' | sed 's/ seconds//'