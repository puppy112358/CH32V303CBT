################################################################################
# MRS Version: 2.4.0
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../example/USB/USBHS/DEVICE/USB_RNDIS_NetWorkAdaptor/User/ETH_Driver/eth_driver_10M.c \
../example/USB/USBHS/DEVICE/USB_RNDIS_NetWorkAdaptor/User/ETH_Driver/eth_driver_CH32V317.c \
../example/USB/USBHS/DEVICE/USB_RNDIS_NetWorkAdaptor/User/ETH_Driver/eth_driver_MII.c \
../example/USB/USBHS/DEVICE/USB_RNDIS_NetWorkAdaptor/User/ETH_Driver/eth_driver_RGMII.c \
../example/USB/USBHS/DEVICE/USB_RNDIS_NetWorkAdaptor/User/ETH_Driver/eth_driver_RMII.c 

C_DEPS += \
./example/USB/USBHS/DEVICE/USB_RNDIS_NetWorkAdaptor/User/ETH_Driver/eth_driver_10M.d \
./example/USB/USBHS/DEVICE/USB_RNDIS_NetWorkAdaptor/User/ETH_Driver/eth_driver_CH32V317.d \
./example/USB/USBHS/DEVICE/USB_RNDIS_NetWorkAdaptor/User/ETH_Driver/eth_driver_MII.d \
./example/USB/USBHS/DEVICE/USB_RNDIS_NetWorkAdaptor/User/ETH_Driver/eth_driver_RGMII.d \
./example/USB/USBHS/DEVICE/USB_RNDIS_NetWorkAdaptor/User/ETH_Driver/eth_driver_RMII.d 

OBJS += \
./example/USB/USBHS/DEVICE/USB_RNDIS_NetWorkAdaptor/User/ETH_Driver/eth_driver_10M.o \
./example/USB/USBHS/DEVICE/USB_RNDIS_NetWorkAdaptor/User/ETH_Driver/eth_driver_CH32V317.o \
./example/USB/USBHS/DEVICE/USB_RNDIS_NetWorkAdaptor/User/ETH_Driver/eth_driver_MII.o \
./example/USB/USBHS/DEVICE/USB_RNDIS_NetWorkAdaptor/User/ETH_Driver/eth_driver_RGMII.o \
./example/USB/USBHS/DEVICE/USB_RNDIS_NetWorkAdaptor/User/ETH_Driver/eth_driver_RMII.o 

DIR_OBJS += \
./example/USB/USBHS/DEVICE/USB_RNDIS_NetWorkAdaptor/User/ETH_Driver/*.o \

DIR_DEPS += \
./example/USB/USBHS/DEVICE/USB_RNDIS_NetWorkAdaptor/User/ETH_Driver/*.d \

DIR_EXPANDS += \
./example/USB/USBHS/DEVICE/USB_RNDIS_NetWorkAdaptor/User/ETH_Driver/*.234r.expand \


# Each subdirectory must supply rules for building sources it contributes
example/USB/USBHS/DEVICE/USB_RNDIS_NetWorkAdaptor/User/ETH_Driver/%.o: ../example/USB/USBHS/DEVICE/USB_RNDIS_NetWorkAdaptor/User/ETH_Driver/%.c
	@	riscv-none-embed-gcc -march=rv32imacxw -mabi=ilp32 -msmall-data-limit=8 -msave-restore -fmax-errors=20 -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -Wuninitialized -g -I"c:/GitProject/CH32V303CBT/Debug" -I"c:/GitProject/CH32V303CBT/Core" -I"c:/GitProject/CH32V303CBT/User" -I"c:/GitProject/CH32V303CBT/Peripheral/inc" -std=gnu99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"

