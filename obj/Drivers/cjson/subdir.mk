################################################################################
# MRS Version: 2.4.0
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Drivers/cjson/cJSON.c 

C_DEPS += \
./Drivers/cjson/cJSON.d 

OBJS += \
./Drivers/cjson/cJSON.o 

DIR_OBJS += \
./Drivers/cjson/*.o \

DIR_DEPS += \
./Drivers/cjson/*.d \

DIR_EXPANDS += \
./Drivers/cjson/*.234r.expand \


# Each subdirectory must supply rules for building sources it contributes
Drivers/cjson/%.o: ../Drivers/cjson/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GNU RISC-V Cross C Compiler'
	riscv-none-embed-gcc -march=rv32imacxw -mabi=ilp32 -msmall-data-limit=8 -msave-restore -fmax-errors=20 -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -Wuninitialized -g -I"c:/GitProject/CH32V303CBT/Debug" -I"c:/GitProject/CH32V303CBT/Core" -I"c:/GitProject/CH32V303CBT/User" -I"c:/GitProject/CH32V303CBT/Peripheral/inc" -std=gnu99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@

