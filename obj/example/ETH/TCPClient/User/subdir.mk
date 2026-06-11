################################################################################
# MRS Version: 2.4.0
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../example/ETH/TCPClient/User/ch32v30x_it.c \
../example/ETH/TCPClient/User/main.c \
../example/ETH/TCPClient/User/system_ch32v30x.c 

C_DEPS += \
./example/ETH/TCPClient/User/ch32v30x_it.d \
./example/ETH/TCPClient/User/main.d \
./example/ETH/TCPClient/User/system_ch32v30x.d 

OBJS += \
./example/ETH/TCPClient/User/ch32v30x_it.o \
./example/ETH/TCPClient/User/main.o \
./example/ETH/TCPClient/User/system_ch32v30x.o 

DIR_OBJS += \
./example/ETH/TCPClient/User/*.o \

DIR_DEPS += \
./example/ETH/TCPClient/User/*.d \

DIR_EXPANDS += \
./example/ETH/TCPClient/User/*.234r.expand \


# Each subdirectory must supply rules for building sources it contributes
example/ETH/TCPClient/User/%.o: ../example/ETH/TCPClient/User/%.c
	@	riscv-none-embed-gcc -march=rv32imacxw -mabi=ilp32 -msmall-data-limit=8 -msave-restore -fmax-errors=20 -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -Wuninitialized -g -I"c:/GitProject/CH32V303CBT/Debug" -I"c:/GitProject/CH32V303CBT/Core" -I"c:/GitProject/CH32V303CBT/User" -I"c:/GitProject/CH32V303CBT/Peripheral/inc" -std=gnu99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"

