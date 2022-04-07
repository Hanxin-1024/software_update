#!/bin/bash

#批量格式化
for f in $(find ./ -name '*.c' -or -name '*.cpp' -or -name '*.h' -or -name '*.hpp' -type f)
do
    astyle --style=ansi -p -n -U -s -H -S -c $f 
done

