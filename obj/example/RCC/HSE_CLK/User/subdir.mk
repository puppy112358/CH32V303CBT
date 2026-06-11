################################################################################
# MRS Version: 2.4.0
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../example/RCC/HSE_CLK/User/ch32v30x_it.c \
../example/RCC/HSE_CLK/User/main.c \
../example/RCC/HSE_CLK/User/system_ch32v30x.c 

C_DEPS += \
./example/RCC/HSE_CLK/User/ch32v30x_it.d \
./example/RCC/HSE_CLK/User/main.d \
./example/RCC/HSE_CLK/User/system_ch32v30x.d 

OBJS += \
./example/RCC/HSE_CLK/User/ch32v30x_it.o \
./example/RCC/HSE_CLK/User/main.o \
./example/RCC/HSE_CLK/User/system_ch32v30x.o 

DIR_OBJS += \
./example/RCC/HSE_CLK/User/*.o \

DIR_DEPS += \
./example/RCC/HSE_CLK/User/*.d \

DIR_EXPANDS += \
./example/RCC/HSE_CLK/User/*.234r.expand \


# Each subdirectory must supply rules for building sources it contributes
example/RCC/HSE_CLK/User/%.o: ../example/RCC/HSE_CLK/User/%.c
	@	riscv-none-embed-gcc -march=rv32imacxw -mabi=ilp32 -msmall-data-limit=8 -msave-restore -fmax-errors=20 -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -Wuninitialized -g -I"c:/GitProject/CH32V303CBT/Debug" -I"c:/GitProject/CH32V303CBT/Core" -I"c:/GitProject/CH32V303CBT/User" -I"c:/GitProject/CH32V303CBT/Peripheral/inc" -std=gnu99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"

