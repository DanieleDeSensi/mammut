#!/bin/bash
cd archs || exit
tar -xf repara.tar.gz
tar -xf c8.tar.gz
cd ..
$1
rm -rf archs/repara # Remove folder after test since it becomes dirty
rm -rf archs/c8 # Remove folder after test since it becomes dirty
