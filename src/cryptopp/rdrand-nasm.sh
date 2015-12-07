#!/bin/sh

reset

nasm -f elf32 rdrand.S -DX86 -g -o rdrand-x86.o
nasm -f elfx32 rdrand.S -DX32 -g -o rdrand-x32.o
nasm -f elf64 rdrand.S -DX64 -g -o rdrand-x64.o

echo "**************************************"
echo "**************************************"

objdump --disassemble rdrand-x86.o

echo
echo "**************************************"
echo "**************************************"

objdump --disassemble rdrand-x32.o

echo
echo "**************************************"
echo "**************************************"

objdump --disassemble rdrand-x64.o
