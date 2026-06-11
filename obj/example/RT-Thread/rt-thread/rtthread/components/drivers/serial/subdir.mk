################################################################################
# MRS Version: 2.4.0
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../example/RT-Thread/rt-thread/rtthread/components/drivers/serial/serial.c 

C_DEPS += \
./example/RT-Thread/rt-thread/rtthread/components/drivers/serial/serial.d 

OBJS += \
./example/RT-Thread/rt-thread/rtthread/components/drivers/serial/serial.o 

DIR_OBJS += \
./example/RT-Thread/rt-thread/rtthread/components/drivers/serial/*.o \

DIR_DEPS += \
./example/RT-Thread/rt-thread/rtthread/components/drivers/serial/*.d \

DIR_EXPANDS += \
./example/RT-Thread/rt-thread/rtthread/components/drivers/serial/*.234r.expand \


# Each subdirectory must supply rules for building sources it contributes
example/RT-Thread/rt-thread/rtthread/components/drivers/serial/%.o: ../example/RT-Thread/rt-thread/rtthread/components/drivers/serial/%.c
	@	riscv-none-embed-gcc -march=rv32imacxw -mabi=ilp32 -msmall-data-limit=8 -msave-restore -fmax-errors=20 -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -Wuninitialized -g -I"c:/GitProject/CH32V303CBT/Debug" -I"c:/GitProject/CH32V303CBT/Core" -I"c:/GitProject/CH32V303CBT/User" -I"c:/GitProject/CH32V303CBT/Peripheral/inc" -std=gnu99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"

