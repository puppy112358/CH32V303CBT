################################################################################
# MRS Version: 2.4.0
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../example/ETH/IoCHub/IoCHub_UART/app/app_config.c \
../example/ETH/IoCHub/IoCHub_UART/app/app_iochub.c \
../example/ETH/IoCHub/IoCHub_UART/app/app_net.c \
../example/ETH/IoCHub/IoCHub_UART/app/app_uart.c 

C_DEPS += \
./example/ETH/IoCHub/IoCHub_UART/app/app_config.d \
./example/ETH/IoCHub/IoCHub_UART/app/app_iochub.d \
./example/ETH/IoCHub/IoCHub_UART/app/app_net.d \
./example/ETH/IoCHub/IoCHub_UART/app/app_uart.d 

OBJS += \
./example/ETH/IoCHub/IoCHub_UART/app/app_config.o \
./example/ETH/IoCHub/IoCHub_UART/app/app_iochub.o \
./example/ETH/IoCHub/IoCHub_UART/app/app_net.o \
./example/ETH/IoCHub/IoCHub_UART/app/app_uart.o 

DIR_OBJS += \
./example/ETH/IoCHub/IoCHub_UART/app/*.o \

DIR_DEPS += \
./example/ETH/IoCHub/IoCHub_UART/app/*.d \

DIR_EXPANDS += \
./example/ETH/IoCHub/IoCHub_UART/app/*.234r.expand \


# Each subdirectory must supply rules for building sources it contributes
example/ETH/IoCHub/IoCHub_UART/app/%.o: ../example/ETH/IoCHub/IoCHub_UART/app/%.c
	@	riscv-none-embed-gcc -march=rv32imacxw -mabi=ilp32 -msmall-data-limit=8 -msave-restore -fmax-errors=20 -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -Wuninitialized -g -I"c:/GitProject/CH32V303CBT/Debug" -I"c:/GitProject/CH32V303CBT/Core" -I"c:/GitProject/CH32V303CBT/User" -I"c:/GitProject/CH32V303CBT/Peripheral/inc" -std=gnu99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"

