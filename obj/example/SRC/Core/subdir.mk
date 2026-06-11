################################################################################
# MRS Version: 2.4.0
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../example/SRC/Core/core_riscv.c 

C_DEPS += \
./example/SRC/Core/core_riscv.d 

OBJS += \
./example/SRC/Core/core_riscv.o 

DIR_OBJS += \
./example/SRC/Core/*.o \

DIR_DEPS += \
./example/SRC/Core/*.d \

DIR_EXPANDS += \
./example/SRC/Core/*.234r.expand \


# Each subdirectory must supply rules for building sources it contributes
example/SRC/Core/%.o: ../example/SRC/Core/%.c
	@	riscv-none-embed-gcc -march=rv32imacxw -mabi=ilp32 -msmall-data-limit=8 -msave-restore -fmax-errors=20 -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -Wuninitialized -g -I"c:/GitProject/CH32V303CBT/Debug" -I"c:/GitProject/CH32V303CBT/Core" -I"c:/GitProject/CH32V303CBT/User" -I"c:/GitProject/CH32V303CBT/Peripheral/inc" -std=gnu99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"

