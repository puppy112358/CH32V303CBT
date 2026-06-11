################################################################################
# MRS Version: 2.4.0
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../example/USB/USBHS/Udisk_Lib/CH32V30xUFI.c 

C_DEPS += \
./example/USB/USBHS/Udisk_Lib/CH32V30xUFI.d 

OBJS += \
./example/USB/USBHS/Udisk_Lib/CH32V30xUFI.o 

DIR_OBJS += \
./example/USB/USBHS/Udisk_Lib/*.o \

DIR_DEPS += \
./example/USB/USBHS/Udisk_Lib/*.d \

DIR_EXPANDS += \
./example/USB/USBHS/Udisk_Lib/*.234r.expand \


# Each subdirectory must supply rules for building sources it contributes
example/USB/USBHS/Udisk_Lib/%.o: ../example/USB/USBHS/Udisk_Lib/%.c
	@	riscv-none-embed-gcc -march=rv32imacxw -mabi=ilp32 -msmall-data-limit=8 -msave-restore -fmax-errors=20 -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -Wuninitialized -g -I"c:/GitProject/CH32V303CBT/Debug" -I"c:/GitProject/CH32V303CBT/Core" -I"c:/GitProject/CH32V303CBT/User" -I"c:/GitProject/CH32V303CBT/Peripheral/inc" -std=gnu99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"

