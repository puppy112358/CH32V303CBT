################################################################################
# MRS Version: 2.4.0
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../example/TencentOS/TencentOS/TencentOS_Tiny/kernel/pm/tos_pm.c \
../example/TencentOS/TencentOS/TencentOS_Tiny/kernel/pm/tos_tickless.c 

C_DEPS += \
./example/TencentOS/TencentOS/TencentOS_Tiny/kernel/pm/tos_pm.d \
./example/TencentOS/TencentOS/TencentOS_Tiny/kernel/pm/tos_tickless.d 

OBJS += \
./example/TencentOS/TencentOS/TencentOS_Tiny/kernel/pm/tos_pm.o \
./example/TencentOS/TencentOS/TencentOS_Tiny/kernel/pm/tos_tickless.o 

DIR_OBJS += \
./example/TencentOS/TencentOS/TencentOS_Tiny/kernel/pm/*.o \

DIR_DEPS += \
./example/TencentOS/TencentOS/TencentOS_Tiny/kernel/pm/*.d \

DIR_EXPANDS += \
./example/TencentOS/TencentOS/TencentOS_Tiny/kernel/pm/*.234r.expand \


# Each subdirectory must supply rules for building sources it contributes
example/TencentOS/TencentOS/TencentOS_Tiny/kernel/pm/%.o: ../example/TencentOS/TencentOS/TencentOS_Tiny/kernel/pm/%.c
	@	riscv-none-embed-gcc -march=rv32imacxw -mabi=ilp32 -msmall-data-limit=8 -msave-restore -fmax-errors=20 -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -Wuninitialized -g -I"c:/GitProject/CH32V303CBT/Debug" -I"c:/GitProject/CH32V303CBT/Core" -I"c:/GitProject/CH32V303CBT/User" -I"c:/GitProject/CH32V303CBT/Peripheral/inc" -std=gnu99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"

