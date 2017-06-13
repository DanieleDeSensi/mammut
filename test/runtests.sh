#!/bin/bash

cd archs && tar -xvf repara.tar.gz && rm -f repara.tar.gz
cd ..
for t in *.cpp; do
	echo $t
	./$(basename "$t" .cpp)
done
