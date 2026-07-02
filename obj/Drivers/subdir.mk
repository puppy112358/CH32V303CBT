################################################################################
# MRS Version: 2.5.0
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Drivers/ch32v30x_usbfs_device.c \
../Drivers/dac8571.c \
../Drivers/fan.c \
../Drivers/fault.c \
../Drivers/i2c_util.c \
../Drivers/ina226.c \
../Drivers/pid.c \
../Drivers/protocol.c \
../Drivers/temp_sensor.c \
../Drivers/usb_cdc.c \
../Drivers/usb_desc.c \
../Drivers/ws2812.c 

C_DEPS += \
./Drivers/ch32v30x_usbfs_device.d \
./Drivers/dac8571.d \
./Drivers/fan.d \
./Drivers/fault.d \
./Drivers/i2c_util.d \
./Drivers/ina226.d \
./Drivers/pid.d \
./Drivers/protocol.d \
./Drivers/temp_sensor.d \
./Drivers/usb_cdc.d \
./Drivers/usb_desc.d \
./Drivers/ws2812.d 

OBJS += \
./Drivers/ch32v30x_usbfs_device.o \
./Drivers/dac8571.o \
./Drivers/fan.o \
./Drivers/fault.o \
./Drivers/i2c_util.o \
./Drivers/ina226.o \
./Drivers/pid.o \
./Drivers/protocol.o \
./Drivers/temp_sensor.o \
./Drivers/usb_cdc.o \
./Drivers/usb_desc.o \
./Drivers/ws2812.o 

DIR_OBJS += \
./Drivers/*.o \

DIR_DEPS += \
./Drivers/*.d \

DIR_EXPANDS += \
./Drivers/*.234r.expand \


# Each subdirectory must supply rules for building sources it contributes
Drivers/%.o: ../Drivers/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GNU RISC-V Cross C Compiler'
	riscv-none-embed-gcc -march=rv32imacxw -mabi=ilp32 -msmall-data-limit=8 -msave-restore -fmax-errors=20 -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -Wuninitialized -g -I"c:/GitProject/CH32V303CBT/Debug" -I"c:/GitProject/CH32V303CBT/Core" -I"c:/GitProject/CH32V303CBT/User" -I"c:/GitProject/CH32V303CBT/Peripheral/inc" -std=gnu99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@

