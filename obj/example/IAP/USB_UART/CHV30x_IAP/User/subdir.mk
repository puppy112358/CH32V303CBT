################################################################################
# MRS Version: 2.4.0
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../example/IAP/USB_UART/CHV30x_IAP/User/ch32v30x_it.c \
../example/IAP/USB_UART/CHV30x_IAP/User/ch32v30x_usbfs_device.c \
../example/IAP/USB_UART/CHV30x_IAP/User/ch32v30x_usbhs_device.c \
../example/IAP/USB_UART/CHV30x_IAP/User/flash.c \
../example/IAP/USB_UART/CHV30x_IAP/User/iap.c \
../example/IAP/USB_UART/CHV30x_IAP/User/main.c \
../example/IAP/USB_UART/CHV30x_IAP/User/system_ch32v30x.c 

C_DEPS += \
./example/IAP/USB_UART/CHV30x_IAP/User/ch32v30x_it.d \
./example/IAP/USB_UART/CHV30x_IAP/User/ch32v30x_usbfs_device.d \
./example/IAP/USB_UART/CHV30x_IAP/User/ch32v30x_usbhs_device.d \
./example/IAP/USB_UART/CHV30x_IAP/User/flash.d \
./example/IAP/USB_UART/CHV30x_IAP/User/iap.d \
./example/IAP/USB_UART/CHV30x_IAP/User/main.d \
./example/IAP/USB_UART/CHV30x_IAP/User/system_ch32v30x.d 

OBJS += \
./example/IAP/USB_UART/CHV30x_IAP/User/ch32v30x_it.o \
./example/IAP/USB_UART/CHV30x_IAP/User/ch32v30x_usbfs_device.o \
./example/IAP/USB_UART/CHV30x_IAP/User/ch32v30x_usbhs_device.o \
./example/IAP/USB_UART/CHV30x_IAP/User/flash.o \
./example/IAP/USB_UART/CHV30x_IAP/User/iap.o \
./example/IAP/USB_UART/CHV30x_IAP/User/main.o \
./example/IAP/USB_UART/CHV30x_IAP/User/system_ch32v30x.o 

DIR_OBJS += \
./example/IAP/USB_UART/CHV30x_IAP/User/*.o \

DIR_DEPS += \
./example/IAP/USB_UART/CHV30x_IAP/User/*.d \

DIR_EXPANDS += \
./example/IAP/USB_UART/CHV30x_IAP/User/*.234r.expand \


# Each subdirectory must supply rules for building sources it contributes
example/IAP/USB_UART/CHV30x_IAP/User/%.o: ../example/IAP/USB_UART/CHV30x_IAP/User/%.c
	@	riscv-none-embed-gcc -march=rv32imacxw -mabi=ilp32 -msmall-data-limit=8 -msave-restore -fmax-errors=20 -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -Wuninitialized -g -I"c:/GitProject/CH32V303CBT/Debug" -I"c:/GitProject/CH32V303CBT/Core" -I"c:/GitProject/CH32V303CBT/User" -I"c:/GitProject/CH32V303CBT/Peripheral/inc" -std=gnu99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"

