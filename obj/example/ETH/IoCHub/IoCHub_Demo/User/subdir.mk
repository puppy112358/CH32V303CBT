################################################################################
# MRS Version: 2.4.0
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../example/ETH/IoCHub/IoCHub_Demo/User/ch32v30x_it.c \
../example/ETH/IoCHub/IoCHub_Demo/User/main.c \
../example/ETH/IoCHub/IoCHub_Demo/User/system_ch32v30x.c 

C_DEPS += \
./example/ETH/IoCHub/IoCHub_Demo/User/ch32v30x_it.d \
./example/ETH/IoCHub/IoCHub_Demo/User/main.d \
./example/ETH/IoCHub/IoCHub_Demo/User/system_ch32v30x.d 

OBJS += \
./example/ETH/IoCHub/IoCHub_Demo/User/ch32v30x_it.o \
./example/ETH/IoCHub/IoCHub_Demo/User/main.o \
./example/ETH/IoCHub/IoCHub_Demo/User/system_ch32v30x.o 

DIR_OBJS += \
./example/ETH/IoCHub/IoCHub_Demo/User/*.o \

DIR_DEPS += \
./example/ETH/IoCHub/IoCHub_Demo/User/*.d \

DIR_EXPANDS += \
./example/ETH/IoCHub/IoCHub_Demo/User/*.234r.expand \


# Each subdirectory must supply rules for building sources it contributes
example/ETH/IoCHub/IoCHub_Demo/User/%.o: ../example/ETH/IoCHub/IoCHub_Demo/User/%.c
	@	riscv-none-embed-gcc -march=rv32imacxw -mabi=ilp32 -msmall-data-limit=8 -msave-restore -fmax-errors=20 -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -Wuninitialized -g -I"c:/GitProject/CH32V303CBT/Debug" -I"c:/GitProject/CH32V303CBT/Core" -I"c:/GitProject/CH32V303CBT/User" -I"c:/GitProject/CH32V303CBT/Peripheral/inc" -std=gnu99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"

