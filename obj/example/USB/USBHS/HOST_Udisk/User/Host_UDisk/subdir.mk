################################################################################
# MRS Version: 2.4.0
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../example/USB/USBHS/HOST_Udisk/User/Host_UDisk/UDisk_Func_CreatDir.c \
../example/USB/USBHS/HOST_Udisk/User/Host_UDisk/UDisk_Func_LongName.c \
../example/USB/USBHS/HOST_Udisk/User/Host_UDisk/UDisk_HW.c \
../example/USB/USBHS/HOST_Udisk/User/Host_UDisk/Udisk_Func_BasicOp.c 

C_DEPS += \
./example/USB/USBHS/HOST_Udisk/User/Host_UDisk/UDisk_Func_CreatDir.d \
./example/USB/USBHS/HOST_Udisk/User/Host_UDisk/UDisk_Func_LongName.d \
./example/USB/USBHS/HOST_Udisk/User/Host_UDisk/UDisk_HW.d \
./example/USB/USBHS/HOST_Udisk/User/Host_UDisk/Udisk_Func_BasicOp.d 

OBJS += \
./example/USB/USBHS/HOST_Udisk/User/Host_UDisk/UDisk_Func_CreatDir.o \
./example/USB/USBHS/HOST_Udisk/User/Host_UDisk/UDisk_Func_LongName.o \
./example/USB/USBHS/HOST_Udisk/User/Host_UDisk/UDisk_HW.o \
./example/USB/USBHS/HOST_Udisk/User/Host_UDisk/Udisk_Func_BasicOp.o 

DIR_OBJS += \
./example/USB/USBHS/HOST_Udisk/User/Host_UDisk/*.o \

DIR_DEPS += \
./example/USB/USBHS/HOST_Udisk/User/Host_UDisk/*.d \

DIR_EXPANDS += \
./example/USB/USBHS/HOST_Udisk/User/Host_UDisk/*.234r.expand \


# Each subdirectory must supply rules for building sources it contributes
example/USB/USBHS/HOST_Udisk/User/Host_UDisk/%.o: ../example/USB/USBHS/HOST_Udisk/User/Host_UDisk/%.c
	@	riscv-none-embed-gcc -march=rv32imacxw -mabi=ilp32 -msmall-data-limit=8 -msave-restore -fmax-errors=20 -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -Wuninitialized -g -I"c:/GitProject/CH32V303CBT/Debug" -I"c:/GitProject/CH32V303CBT/Core" -I"c:/GitProject/CH32V303CBT/User" -I"c:/GitProject/CH32V303CBT/Peripheral/inc" -std=gnu99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"

