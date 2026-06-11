################################################################################
# MRS Version: 2.4.0
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../example/FSMC/SRAM/User/ch32v30x_it.c \
../example/FSMC/SRAM/User/main.c \
../example/FSMC/SRAM/User/system_ch32v30x.c 

C_DEPS += \
./example/FSMC/SRAM/User/ch32v30x_it.d \
./example/FSMC/SRAM/User/main.d \
./example/FSMC/SRAM/User/system_ch32v30x.d 

OBJS += \
./example/FSMC/SRAM/User/ch32v30x_it.o \
./example/FSMC/SRAM/User/main.o \
./example/FSMC/SRAM/User/system_ch32v30x.o 

DIR_OBJS += \
./example/FSMC/SRAM/User/*.o \

DIR_DEPS += \
./example/FSMC/SRAM/User/*.d \

DIR_EXPANDS += \
./example/FSMC/SRAM/User/*.234r.expand \


# Each subdirectory must supply rules for building sources it contributes
example/FSMC/SRAM/User/%.o: ../example/FSMC/SRAM/User/%.c
	@	riscv-none-embed-gcc -march=rv32imacxw -mabi=ilp32 -msmall-data-limit=8 -msave-restore -fmax-errors=20 -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -Wuninitialized -g -I"c:/GitProject/CH32V303CBT/Debug" -I"c:/GitProject/CH32V303CBT/Core" -I"c:/GitProject/CH32V303CBT/User" -I"c:/GitProject/CH32V303CBT/Peripheral/inc" -std=gnu99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"

