@echo off

cd ..
rmdir /S /Q build
mkdir build 
cd build
cmake -DTESTS=ON -DGLFW_ROOT_DIR=../lib/win64/glfw ..
cd ../tools
