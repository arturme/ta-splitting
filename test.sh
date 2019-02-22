#!/bin/sh

if [ "$1" != "n" ];then
	make clean all
fi

time ./verifier < netfiles/tgc.aut
	
