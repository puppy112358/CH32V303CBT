################################################################################
# MRS Version: 2.4.0
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../example/USB/USBHS/DEVICE/CompositeKM/User/ch32v30x_it.c \
../example/USB/USBHS/DEVICE/CompositeKM/User/ch32v30x_usbhs_device.c \
../example/USB/USBHS/DEVICE/CompositeKM/User/main.c \
../example/USB/USBHS/DEVICE/CompositeKM/User/system_ch32v30x.c \
../example/USB/USBHS/DEVICE/CompositeKM/User/usbd_compostie_km.c \
../example/USB/USBHS/DEVICE/CompositeKM/User/usbd_desc.c 

C_DEPS += \
./example/USB/USBHS/DEVICE/CompositeKM/User/ch32v30x_it.d \
./example/USB/USBHS/DEVICE/CompositeKM/User/ch32v30x_usbhs_device.d \
./example/USB/USBHS/DEVICE/CompositeKM/User/main.d \
./example/USB/USBHS/DEVICE/CompositeKM/User/system_ch32v30x.d \
./example/USB/USBHS/DEVICE/CompositeKM/User/usbd_compostie_km.d \
./example/USB/USBHS/DEVICE/CompositeKM/User/usbd_desc.d 

OBJS += \
./example/USB/USBHS/DEVICE/CompositeKM/User/ch32v30x_it.o \
./example/USB/USBHS/DEVICE/CompositeKM/User/ch32v30x_usbhs_device.o \
./example/USB/USBHS/DEVICE/CompositeKM/User/main.o \
./example/USB/USBHS/DEVICE/CompositeKM/User/system_ch32v30x.o \
./example/USB/USBHS/DEVICE/CompositeKM/User/usbd_compostie_km.o \
./example/USB/USBHS/DEVICE/CompositeKM/User/usbd_desc.o 

DIR_OBJS += \
./example/USB/USBHS/DEVICE/CompositeKM/User/*.o \

DIR_DEPS += \
./example/USB/USBHS/DEVICE/CompositeKM/User/*.d \

DIR_EXPANDS += \
./example/USB/USBHS/DEVICE/CompositeKM/User/*.234r.expand \


# Each subdirectory must supply rules for building sources it contributes
example/USB/USBHS/DEVICE/CompositeKM/User/%.o: ../example/USB/USBHS/DEVICE/CompositeKM/User/%.c
	@	riscv-none-embed-gcc -march=rv32imacxw -mabi=ilp32 -msmall-data-limit=8 -msave-restore -fmax-errors=20 -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -Wuninitialized -g -I"c:/GitProject/CH32V303CBT/Debug" -I"c:/GitProject/CH32V303CBT/Core" -I"c:/GitProject/CH32V303CBT/User" -I"c:/GitProject/CH32V303CBT/Peripheral/inc" -std=gnu99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"

