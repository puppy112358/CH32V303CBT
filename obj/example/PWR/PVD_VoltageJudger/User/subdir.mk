################################################################################
# MRS Version: 2.4.0
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../example/PWR/PVD_VoltageJudger/User/ch32v30x_it.c \
../example/PWR/PVD_VoltageJudger/User/main.c \
../example/PWR/PVD_VoltageJudger/User/system_ch32v30x.c 

C_DEPS += \
./example/PWR/PVD_VoltageJudger/User/ch32v30x_it.d \
./example/PWR/PVD_VoltageJudger/User/main.d \
./example/PWR/PVD_VoltageJudger/User/system_ch32v30x.d 

OBJS += \
./example/PWR/PVD_VoltageJudger/User/ch32v30x_it.o \
./example/PWR/PVD_VoltageJudger/User/main.o \
./example/PWR/PVD_VoltageJudger/User/system_ch32v30x.o 

DIR_OBJS += \
./example/PWR/PVD_VoltageJudger/User/*.o \

DIR_DEPS += \
./example/PWR/PVD_VoltageJudger/User/*.d \

DIR_EXPANDS += \
./example/PWR/PVD_VoltageJudger/User/*.234r.expand \


# Each subdirectory must supply rules for building sources it contributes
example/PWR/PVD_VoltageJudger/User/%.o: ../example/PWR/PVD_VoltageJudger/User/%.c
	@	riscv-none-embed-gcc -march=rv32imacxw -mabi=ilp32 -msmall-data-limit=8 -msave-restore -fmax-errors=20 -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -Wuninitialized -g -I"c:/GitProject/CH32V303CBT/Debug" -I"c:/GitProject/CH32V303CBT/Core" -I"c:/GitProject/CH32V303CBT/User" -I"c:/GitProject/CH32V303CBT/Peripheral/inc" -std=gnu99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"

