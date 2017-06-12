#!/bin/bash

for t in *.cpp; do
	echo $t
	./$(basename "$t" .cpp)
done