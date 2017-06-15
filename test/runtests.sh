#!/bin/bash

cd archs 
if [ ! -d "repara" ]; then
	tar -xvf repara.tar.gz && rm -f repara.tar.gz
fi
cd ..
for t in *.cpp; do
	echo $t
	./$(basename "$t" .cpp)
done
