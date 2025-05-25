echo off
nasm\nasm -fbin code.asm -o code.img
echo Inserted .img file
echo Loading QEMU
qemu\qemu code.img
