################################################################################
# MRS Version: 2.4.0
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../example/ETH/NetLib/eth_driver_10M.c \
../example/ETH/NetLib/eth_driver_CH32V317.c \
../example/ETH/NetLib/eth_driver_MII.c \
../example/ETH/NetLib/eth_driver_RGMII.c \
../example/ETH/NetLib/eth_driver_RMII.c 

C_DEPS += \
./example/ETH/NetLib/eth_driver_10M.d \
./example/ETH/NetLib/eth_driver_CH32V317.d \
./example/ETH/NetLib/eth_driver_MII.d \
./example/ETH/NetLib/eth_driver_RGMII.d \
./example/ETH/NetLib/eth_driver_RMII.d 

OBJS += \
./example/ETH/NetLib/eth_driver_10M.o \
./example/ETH/NetLib/eth_driver_CH32V317.o \
./example/ETH/NetLib/eth_driver_MII.o \
./example/ETH/NetLib/eth_driver_RGMII.o \
./example/ETH/NetLib/eth_driver_RMII.o 

DIR_OBJS += \
./example/ETH/NetLib/*.o \

DIR_DEPS += \
./example/ETH/NetLib/*.d \

DIR_EXPANDS += \
./example/ETH/NetLib/*.234r.expand \


# Each subdirectory must supply rules for building sources it contributes
example/ETH/NetLib/%.o: ../example/ETH/NetLib/%.c
	@	riscv-none-embed-gcc -march=rv32imacxw -mabi=ilp32 -msmall-data-limit=8 -msave-restore -fmax-errors=20 -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -Wuninitialized -g -I"c:/GitProject/CH32V303CBT/Debug" -I"c:/GitProject/CH32V303CBT/Core" -I"c:/GitProject/CH32V303CBT/User" -I"c:/GitProject/CH32V303CBT/Peripheral/inc" -std=gnu99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"

