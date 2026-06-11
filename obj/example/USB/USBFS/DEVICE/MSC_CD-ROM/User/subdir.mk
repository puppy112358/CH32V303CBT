################################################################################
# MRS Version: 2.4.0
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../example/USB/USBFS/DEVICE/MSC_CD-ROM/User/SPI_FLASH.c \
../example/USB/USBFS/DEVICE/MSC_CD-ROM/User/SW_CDROM.c \
../example/USB/USBFS/DEVICE/MSC_CD-ROM/User/ch32v30x_it.c \
../example/USB/USBFS/DEVICE/MSC_CD-ROM/User/ch32v30x_usbfs_device.c \
../example/USB/USBFS/DEVICE/MSC_CD-ROM/User/main.c \
../example/USB/USBFS/DEVICE/MSC_CD-ROM/User/system_ch32v30x.c \
../example/USB/USBFS/DEVICE/MSC_CD-ROM/User/usb_desc.c 

C_DEPS += \
./example/USB/USBFS/DEVICE/MSC_CD-ROM/User/SPI_FLASH.d \
./example/USB/USBFS/DEVICE/MSC_CD-ROM/User/SW_CDROM.d \
./example/USB/USBFS/DEVICE/MSC_CD-ROM/User/ch32v30x_it.d \
./example/USB/USBFS/DEVICE/MSC_CD-ROM/User/ch32v30x_usbfs_device.d \
./example/USB/USBFS/DEVICE/MSC_CD-ROM/User/main.d \
./example/USB/USBFS/DEVICE/MSC_CD-ROM/User/system_ch32v30x.d \
./example/USB/USBFS/DEVICE/MSC_CD-ROM/User/usb_desc.d 

OBJS += \
./example/USB/USBFS/DEVICE/MSC_CD-ROM/User/SPI_FLASH.o \
./example/USB/USBFS/DEVICE/MSC_CD-ROM/User/SW_CDROM.o \
./example/USB/USBFS/DEVICE/MSC_CD-ROM/User/ch32v30x_it.o \
./example/USB/USBFS/DEVICE/MSC_CD-ROM/User/ch32v30x_usbfs_device.o \
./example/USB/USBFS/DEVICE/MSC_CD-ROM/User/main.o \
./example/USB/USBFS/DEVICE/MSC_CD-ROM/User/system_ch32v30x.o \
./example/USB/USBFS/DEVICE/MSC_CD-ROM/User/usb_desc.o 

DIR_OBJS += \
./example/USB/USBFS/DEVICE/MSC_CD-ROM/User/*.o \

DIR_DEPS += \
./example/USB/USBFS/DEVICE/MSC_CD-ROM/User/*.d \

DIR_EXPANDS += \
./example/USB/USBFS/DEVICE/MSC_CD-ROM/User/*.234r.expand \


# Each subdirectory must supply rules for building sources it contributes
example/USB/USBFS/DEVICE/MSC_CD-ROM/User/%.o: ../example/USB/USBFS/DEVICE/MSC_CD-ROM/User/%.c
	@	riscv-none-embed-gcc -march=rv32imacxw -mabi=ilp32 -msmall-data-limit=8 -msave-restore -fmax-errors=20 -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -Wuninitialized -g -I"c:/GitProject/CH32V303CBT/Debug" -I"c:/GitProject/CH32V303CBT/Core" -I"c:/GitProject/CH32V303CBT/User" -I"c:/GitProject/CH32V303CBT/Peripheral/inc" -std=gnu99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"

