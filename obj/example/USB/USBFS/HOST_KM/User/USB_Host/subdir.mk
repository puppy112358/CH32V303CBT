################################################################################
# MRS Version: 2.4.0
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../example/USB/USBFS/HOST_KM/User/USB_Host/app_km.c \
../example/USB/USBFS/HOST_KM/User/USB_Host/ch32v30x_usbfs_host.c \
../example/USB/USBFS/HOST_KM/User/USB_Host/usb_host_hid.c \
../example/USB/USBFS/HOST_KM/User/USB_Host/usb_host_hub.c 

C_DEPS += \
./example/USB/USBFS/HOST_KM/User/USB_Host/app_km.d \
./example/USB/USBFS/HOST_KM/User/USB_Host/ch32v30x_usbfs_host.d \
./example/USB/USBFS/HOST_KM/User/USB_Host/usb_host_hid.d \
./example/USB/USBFS/HOST_KM/User/USB_Host/usb_host_hub.d 

OBJS += \
./example/USB/USBFS/HOST_KM/User/USB_Host/app_km.o \
./example/USB/USBFS/HOST_KM/User/USB_Host/ch32v30x_usbfs_host.o \
./example/USB/USBFS/HOST_KM/User/USB_Host/usb_host_hid.o \
./example/USB/USBFS/HOST_KM/User/USB_Host/usb_host_hub.o 

DIR_OBJS += \
./example/USB/USBFS/HOST_KM/User/USB_Host/*.o \

DIR_DEPS += \
./example/USB/USBFS/HOST_KM/User/USB_Host/*.d \

DIR_EXPANDS += \
./example/USB/USBFS/HOST_KM/User/USB_Host/*.234r.expand \


# Each subdirectory must supply rules for building sources it contributes
example/USB/USBFS/HOST_KM/User/USB_Host/%.o: ../example/USB/USBFS/HOST_KM/User/USB_Host/%.c
	@	riscv-none-embed-gcc -march=rv32imacxw -mabi=ilp32 -msmall-data-limit=8 -msave-restore -fmax-errors=20 -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -Wuninitialized -g -I"c:/GitProject/CH32V303CBT/Debug" -I"c:/GitProject/CH32V303CBT/Core" -I"c:/GitProject/CH32V303CBT/User" -I"c:/GitProject/CH32V303CBT/Peripheral/inc" -std=gnu99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"

