################################################################################
# MRS Version: 2.4.0
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../example/USB/USBHS/HOST_MTP_FileSystem/User/USB_Host/app_mtp_ptp.c \
../example/USB/USBHS/HOST_MTP_FileSystem/User/USB_Host/ch32v30x_usbfs_host.c \
../example/USB/USBHS/HOST_MTP_FileSystem/User/USB_Host/ch32v30x_usbhs_host.c 

C_DEPS += \
./example/USB/USBHS/HOST_MTP_FileSystem/User/USB_Host/app_mtp_ptp.d \
./example/USB/USBHS/HOST_MTP_FileSystem/User/USB_Host/ch32v30x_usbfs_host.d \
./example/USB/USBHS/HOST_MTP_FileSystem/User/USB_Host/ch32v30x_usbhs_host.d 

OBJS += \
./example/USB/USBHS/HOST_MTP_FileSystem/User/USB_Host/app_mtp_ptp.o \
./example/USB/USBHS/HOST_MTP_FileSystem/User/USB_Host/ch32v30x_usbfs_host.o \
./example/USB/USBHS/HOST_MTP_FileSystem/User/USB_Host/ch32v30x_usbhs_host.o 

DIR_OBJS += \
./example/USB/USBHS/HOST_MTP_FileSystem/User/USB_Host/*.o \

DIR_DEPS += \
./example/USB/USBHS/HOST_MTP_FileSystem/User/USB_Host/*.d \

DIR_EXPANDS += \
./example/USB/USBHS/HOST_MTP_FileSystem/User/USB_Host/*.234r.expand \


# Each subdirectory must supply rules for building sources it contributes
example/USB/USBHS/HOST_MTP_FileSystem/User/USB_Host/%.o: ../example/USB/USBHS/HOST_MTP_FileSystem/User/USB_Host/%.c
	@	riscv-none-embed-gcc -march=rv32imacxw -mabi=ilp32 -msmall-data-limit=8 -msave-restore -fmax-errors=20 -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -Wuninitialized -g -I"c:/GitProject/CH32V303CBT/Debug" -I"c:/GitProject/CH32V303CBT/Core" -I"c:/GitProject/CH32V303CBT/User" -I"c:/GitProject/CH32V303CBT/Peripheral/inc" -std=gnu99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"

