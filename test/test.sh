#!/bin/bash
cd archs || exit
tar -xf repara.tar.gz
cd ..
$1
rm -rf archs/repara # Remove folder after test since it becomes dirty
