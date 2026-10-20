@echo off
setlocal

set CFLAGS=-O2 -mcpu=cortex-a7 -marm -ffreestanding -fno-builtin -fno-common -fno-unwind-tables -fno-asynchronous-unwind-tables -nostdlib -Wall -Wextra

echo Cleaning old files...
del /f /q *.o *.elf *.bin *.map *.lst 2>nul

echo Building startup.S...
arm-none-eabi-gcc -c %CFLAGS% startup.S -o startup.o
if errorlevel 1 goto error

echo Building C sources...
for %%F in (boot_state platform gpio display gt911 hello) do (
    arm-none-eabi-gcc -c %CFLAGS% %%F.c -o %%F.o
    if errorlevel 1 goto error
)

echo Linking hello.elf...
arm-none-eabi-ld -T link.ld -Map=hello.map startup.o boot_state.o platform.o gpio.o display.o gt911.o hello.o -o hello.elf
if errorlevel 1 goto error

echo Creating hello.bin...
arm-none-eabi-objcopy -O binary hello.elf hello.bin
if errorlevel 1 goto error

arm-none-eabi-objdump -d -S hello.elf > hello.lst
if errorlevel 1 goto error

echo.
echo ===== BUILD SUCCESSFUL =====
arm-none-eabi-size hello.elf
echo.
echo Entry point:
arm-none-eabi-readelf -h hello.elf | findstr /C:"Entry point"
echo.
echo Outputs: hello.bin hello.elf hello.map hello.lst
exit /b 0

:error
echo.
echo ===== BUILD FAILED =====
exit /b 1
