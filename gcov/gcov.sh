#!/bin/bash
# Must be called from ../ (i.e. mammut root)

# Remove unneeded coverage files.
for suffix in  *.gcno *.gcda ; do 
	find ./demo -name $$suffix -type f -delete 
	find ./test -name $$suffix -type f -delete 
	find ./mammut/external -name $$suffix -type f -delete 
done
# Get all remaining coverage files.
COVFILES=$(find . -name *.gcda -type f)
CURRENTDIR=$(pwd)
for file in ${COVFILES} ; do 
	cd $(dirname ${file}) 
	gcov -r $(basename ${file})
	cd $CURRENTDIR
done

# Move all the coverage files to the ./gcov folder.
find ./ -name *.gcov -type f -exec cp {} ./gcov \;
