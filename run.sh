#!/bin/bash
gcc -c core.c core.c
#gcc -c net.c net.c
gcc -c pool.c pool.c 
gcc -o main main.c pool.o core.o -lpthread

