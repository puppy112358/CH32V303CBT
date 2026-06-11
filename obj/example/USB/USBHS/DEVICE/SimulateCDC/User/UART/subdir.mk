################################################################################
# MRS Version: 2.4.0
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../example/USB/USBHS/DEVICE/SimulateCDC/User/UART/UART.c 

C_DEPS += \
./example/USB/USBHS/DEVICE/SimulateCDC/User/UART/UART.d 

OBJS += \
./example/USB/USBHS/DEVICE/SimulateCDC/User/UART/UART.o 

DIR_OBJS += \
./example/USB/USBHS/DEVICE/SimulateCDC/User/UART/*.o \

DIR_DEPS += \
./example/USB/USBHS/DEVICE/SimulateCDC/User/UART/*.d \

DIR_EXPANDS += \
./example/USB/USBHS/DEVICE/SimulateCDC/User/UART/*.234r.expand \


# Each subdirectory must supply rules for building sources it contributes
example/USB/USBHS/DEVICE/SimulateCDC/User/UART/%.o: ../example/USB/USBHS/DEVICE/SimulateCDC/User/UART/%.c
	@	riscv-none-embed-gcc -march=rv32imacxw -mabi=ilp32 -msmall-data-limit=8 -msave-restore -fmax-errors=20 -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -Wuninitialized -g -I"c:/GitProject/CH32V303CBT/Debug" -I"c:/GitProject/CH32V303CBT/Core" -I"c:/GitProject/CH32V303CBT/User" -I"c:/GitProject/CH32V303CBT/Peripheral/inc" -std=gnu99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"

