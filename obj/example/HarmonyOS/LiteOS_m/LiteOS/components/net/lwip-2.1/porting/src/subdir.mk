################################################################################
# MRS Version: 2.4.0
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../example/HarmonyOS/LiteOS_m/LiteOS/components/net/lwip-2.1/porting/src/driverif.c \
../example/HarmonyOS/LiteOS_m/LiteOS/components/net/lwip-2.1/porting/src/lwip_init.c \
../example/HarmonyOS/LiteOS_m/LiteOS/components/net/lwip-2.1/porting/src/netdb_porting.c \
../example/HarmonyOS/LiteOS_m/LiteOS/components/net/lwip-2.1/porting/src/sockets_porting.c \
../example/HarmonyOS/LiteOS_m/LiteOS/components/net/lwip-2.1/porting/src/sys_arch.c 

C_DEPS += \
./example/HarmonyOS/LiteOS_m/LiteOS/components/net/lwip-2.1/porting/src/driverif.d \
./example/HarmonyOS/LiteOS_m/LiteOS/components/net/lwip-2.1/porting/src/lwip_init.d \
./example/HarmonyOS/LiteOS_m/LiteOS/components/net/lwip-2.1/porting/src/netdb_porting.d \
./example/HarmonyOS/LiteOS_m/LiteOS/components/net/lwip-2.1/porting/src/sockets_porting.d \
./example/HarmonyOS/LiteOS_m/LiteOS/components/net/lwip-2.1/porting/src/sys_arch.d 

OBJS += \
./example/HarmonyOS/LiteOS_m/LiteOS/components/net/lwip-2.1/porting/src/driverif.o \
./example/HarmonyOS/LiteOS_m/LiteOS/components/net/lwip-2.1/porting/src/lwip_init.o \
./example/HarmonyOS/LiteOS_m/LiteOS/components/net/lwip-2.1/porting/src/netdb_porting.o \
./example/HarmonyOS/LiteOS_m/LiteOS/components/net/lwip-2.1/porting/src/sockets_porting.o \
./example/HarmonyOS/LiteOS_m/LiteOS/components/net/lwip-2.1/porting/src/sys_arch.o 

DIR_OBJS += \
./example/HarmonyOS/LiteOS_m/LiteOS/components/net/lwip-2.1/porting/src/*.o \

DIR_DEPS += \
./example/HarmonyOS/LiteOS_m/LiteOS/components/net/lwip-2.1/porting/src/*.d \

DIR_EXPANDS += \
./example/HarmonyOS/LiteOS_m/LiteOS/components/net/lwip-2.1/porting/src/*.234r.expand \


# Each subdirectory must supply rules for building sources it contributes
example/HarmonyOS/LiteOS_m/LiteOS/components/net/lwip-2.1/porting/src/%.o: ../example/HarmonyOS/LiteOS_m/LiteOS/components/net/lwip-2.1/porting/src/%.c
	@	riscv-none-embed-gcc -march=rv32imacxw -mabi=ilp32 -msmall-data-limit=8 -msave-restore -fmax-errors=20 -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -Wuninitialized -g -I"c:/GitProject/CH32V303CBT/Debug" -I"c:/GitProject/CH32V303CBT/Core" -I"c:/GitProject/CH32V303CBT/User" -I"c:/GitProject/CH32V303CBT/Peripheral/inc" -std=gnu99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"

