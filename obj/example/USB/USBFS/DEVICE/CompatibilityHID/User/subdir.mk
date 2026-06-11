################################################################################
# MRS Version: 2.4.0
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../example/USB/USBFS/DEVICE/CompatibilityHID/User/ch32v30x_it.c \
../example/USB/USBFS/DEVICE/CompatibilityHID/User/ch32v30x_usbfs_device.c \
../example/USB/USBFS/DEVICE/CompatibilityHID/User/main.c \
../example/USB/USBFS/DEVICE/CompatibilityHID/User/system_ch32v30x.c \
../example/USB/USBFS/DEVICE/CompatibilityHID/User/usb_desc.c \
../example/USB/USBFS/DEVICE/CompatibilityHID/User/usbd_compatibility_hid.c 

C_DEPS += \
./example/USB/USBFS/DEVICE/CompatibilityHID/User/ch32v30x_it.d \
./example/USB/USBFS/DEVICE/CompatibilityHID/User/ch32v30x_usbfs_device.d \
./example/USB/USBFS/DEVICE/CompatibilityHID/User/main.d \
./example/USB/USBFS/DEVICE/CompatibilityHID/User/system_ch32v30x.d \
./example/USB/USBFS/DEVICE/CompatibilityHID/User/usb_desc.d \
./example/USB/USBFS/DEVICE/CompatibilityHID/User/usbd_compatibility_hid.d 

OBJS += \
./example/USB/USBFS/DEVICE/CompatibilityHID/User/ch32v30x_it.o \
./example/USB/USBFS/DEVICE/CompatibilityHID/User/ch32v30x_usbfs_device.o \
./example/USB/USBFS/DEVICE/CompatibilityHID/User/main.o \
./example/USB/USBFS/DEVICE/CompatibilityHID/User/system_ch32v30x.o \
./example/USB/USBFS/DEVICE/CompatibilityHID/User/usb_desc.o \
./example/USB/USBFS/DEVICE/CompatibilityHID/User/usbd_compatibility_hid.o 

DIR_OBJS += \
./example/USB/USBFS/DEVICE/CompatibilityHID/User/*.o \

DIR_DEPS += \
./example/USB/USBFS/DEVICE/CompatibilityHID/User/*.d \

DIR_EXPANDS += \
./example/USB/USBFS/DEVICE/CompatibilityHID/User/*.234r.expand \


# Each subdirectory must supply rules for building sources it contributes
example/USB/USBFS/DEVICE/CompatibilityHID/User/%.o: ../example/USB/USBFS/DEVICE/CompatibilityHID/User/%.c
	@	riscv-none-embed-gcc -march=rv32imacxw -mabi=ilp32 -msmall-data-limit=8 -msave-restore -fmax-errors=20 -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -Wuninitialized -g -I"c:/GitProject/CH32V303CBT/Debug" -I"c:/GitProject/CH32V303CBT/Core" -I"c:/GitProject/CH32V303CBT/User" -I"c:/GitProject/CH32V303CBT/Peripheral/inc" -std=gnu99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"

