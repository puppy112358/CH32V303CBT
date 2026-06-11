################################################################################
# MRS Version: 2.4.0
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../example/FSMC/LCD_8bit/User/ch32v30x_it.c \
../example/FSMC/LCD_8bit/User/lcd.c \
../example/FSMC/LCD_8bit/User/lcd_init.c \
../example/FSMC/LCD_8bit/User/main.c \
../example/FSMC/LCD_8bit/User/system_ch32v30x.c 

C_DEPS += \
./example/FSMC/LCD_8bit/User/ch32v30x_it.d \
./example/FSMC/LCD_8bit/User/lcd.d \
./example/FSMC/LCD_8bit/User/lcd_init.d \
./example/FSMC/LCD_8bit/User/main.d \
./example/FSMC/LCD_8bit/User/system_ch32v30x.d 

OBJS += \
./example/FSMC/LCD_8bit/User/ch32v30x_it.o \
./example/FSMC/LCD_8bit/User/lcd.o \
./example/FSMC/LCD_8bit/User/lcd_init.o \
./example/FSMC/LCD_8bit/User/main.o \
./example/FSMC/LCD_8bit/User/system_ch32v30x.o 

DIR_OBJS += \
./example/FSMC/LCD_8bit/User/*.o \

DIR_DEPS += \
./example/FSMC/LCD_8bit/User/*.d \

DIR_EXPANDS += \
./example/FSMC/LCD_8bit/User/*.234r.expand \


# Each subdirectory must supply rules for building sources it contributes
example/FSMC/LCD_8bit/User/%.o: ../example/FSMC/LCD_8bit/User/%.c
	@	riscv-none-embed-gcc -march=rv32imacxw -mabi=ilp32 -msmall-data-limit=8 -msave-restore -fmax-errors=20 -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -Wuninitialized -g -I"c:/GitProject/CH32V303CBT/Debug" -I"c:/GitProject/CH32V303CBT/Core" -I"c:/GitProject/CH32V303CBT/User" -I"c:/GitProject/CH32V303CBT/Peripheral/inc" -std=gnu99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"

