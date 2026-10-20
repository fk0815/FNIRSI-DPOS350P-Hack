@echo off
echo Cleaning old files...
del /f /q *.o *.elf *.bin 2>nul

echo Building hello.bin...
arm-none-eabi-gcc -c -O2 -mcpu=cortex-a7 -marm -ffreestanding -nostdlib startup.S -o startup.o
arm-none-eabi-gcc -c -O2 -mcpu=cortex-a7 -marm -ffreestanding -nostdlib hello.c -o hello.o
arm-none-eabi-ld -T link.ld startup.o hello.o -o hello.elf
arm-none-eabi-objcopy -O binary hello.elf hello.bin

echo Done: hello.bin
arm-none-eabi-size hello.elf
