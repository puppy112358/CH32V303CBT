################################################################################
# MRS Version: 2.4.0
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../example/USB/USBHS/DEVICE/MSC_U-Disk/User/Internal_Flash.c \
../example/USB/USBHS/DEVICE/MSC_U-Disk/User/SPI_FLASH.c \
../example/USB/USBHS/DEVICE/MSC_U-Disk/User/SW_UDISK.c \
../example/USB/USBHS/DEVICE/MSC_U-Disk/User/ch32v30x_it.c \
../example/USB/USBHS/DEVICE/MSC_U-Disk/User/ch32v30x_usbhs_device.c \
../example/USB/USBHS/DEVICE/MSC_U-Disk/User/main.c \
../example/USB/USBHS/DEVICE/MSC_U-Disk/User/system_ch32v30x.c \
../example/USB/USBHS/DEVICE/MSC_U-Disk/User/usb_desc.c 

C_DEPS += \
./example/USB/USBHS/DEVICE/MSC_U-Disk/User/Internal_Flash.d \
./example/USB/USBHS/DEVICE/MSC_U-Disk/User/SPI_FLASH.d \
./example/USB/USBHS/DEVICE/MSC_U-Disk/User/SW_UDISK.d \
./example/USB/USBHS/DEVICE/MSC_U-Disk/User/ch32v30x_it.d \
./example/USB/USBHS/DEVICE/MSC_U-Disk/User/ch32v30x_usbhs_device.d \
./example/USB/USBHS/DEVICE/MSC_U-Disk/User/main.d \
./example/USB/USBHS/DEVICE/MSC_U-Disk/User/system_ch32v30x.d \
./example/USB/USBHS/DEVICE/MSC_U-Disk/User/usb_desc.d 

OBJS += \
./example/USB/USBHS/DEVICE/MSC_U-Disk/User/Internal_Flash.o \
./example/USB/USBHS/DEVICE/MSC_U-Disk/User/SPI_FLASH.o \
./example/USB/USBHS/DEVICE/MSC_U-Disk/User/SW_UDISK.o \
./example/USB/USBHS/DEVICE/MSC_U-Disk/User/ch32v30x_it.o \
./example/USB/USBHS/DEVICE/MSC_U-Disk/User/ch32v30x_usbhs_device.o \
./example/USB/USBHS/DEVICE/MSC_U-Disk/User/main.o \
./example/USB/USBHS/DEVICE/MSC_U-Disk/User/system_ch32v30x.o \
./example/USB/USBHS/DEVICE/MSC_U-Disk/User/usb_desc.o 

DIR_OBJS += \
./example/USB/USBHS/DEVICE/MSC_U-Disk/User/*.o \

DIR_DEPS += \
./example/USB/USBHS/DEVICE/MSC_U-Disk/User/*.d \

DIR_EXPANDS += \
./example/USB/USBHS/DEVICE/MSC_U-Disk/User/*.234r.expand \


# Each subdirectory must supply rules for building sources it contributes
example/USB/USBHS/DEVICE/MSC_U-Disk/User/%.o: ../example/USB/USBHS/DEVICE/MSC_U-Disk/User/%.c
	@	riscv-none-embed-gcc -march=rv32imacxw -mabi=ilp32 -msmall-data-limit=8 -msave-restore -fmax-errors=20 -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -Wuninitialized -g -I"c:/GitProject/CH32V303CBT/Debug" -I"c:/GitProject/CH32V303CBT/Core" -I"c:/GitProject/CH32V303CBT/User" -I"c:/GitProject/CH32V303CBT/Peripheral/inc" -std=gnu99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"

