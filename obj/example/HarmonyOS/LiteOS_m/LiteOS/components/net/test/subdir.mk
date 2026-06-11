################################################################################
# MRS Version: 2.4.0
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../example/HarmonyOS/LiteOS_m/LiteOS/components/net/test/net_socket_test_001.c \
../example/HarmonyOS/LiteOS_m/LiteOS/components/net/test/net_socket_test_002.c \
../example/HarmonyOS/LiteOS_m/LiteOS/components/net/test/net_socket_test_003.c \
../example/HarmonyOS/LiteOS_m/LiteOS/components/net/test/net_socket_test_004.c \
../example/HarmonyOS/LiteOS_m/LiteOS/components/net/test/net_socket_test_005.c \
../example/HarmonyOS/LiteOS_m/LiteOS/components/net/test/net_socket_test_006.c \
../example/HarmonyOS/LiteOS_m/LiteOS/components/net/test/net_socket_test_007.c \
../example/HarmonyOS/LiteOS_m/LiteOS/components/net/test/net_socket_test_008.c \
../example/HarmonyOS/LiteOS_m/LiteOS/components/net/test/net_socket_test_009.c \
../example/HarmonyOS/LiteOS_m/LiteOS/components/net/test/net_socket_test_010.c \
../example/HarmonyOS/LiteOS_m/LiteOS/components/net/test/net_socket_test_011.c \
../example/HarmonyOS/LiteOS_m/LiteOS/components/net/test/net_socket_test_012.c \
../example/HarmonyOS/LiteOS_m/LiteOS/components/net/test/net_socket_test_013.c \
../example/HarmonyOS/LiteOS_m/LiteOS/components/net/test/test_main.c 

C_DEPS += \
./example/HarmonyOS/LiteOS_m/LiteOS/components/net/test/net_socket_test_001.d \
./example/HarmonyOS/LiteOS_m/LiteOS/components/net/test/net_socket_test_002.d \
./example/HarmonyOS/LiteOS_m/LiteOS/components/net/test/net_socket_test_003.d \
./example/HarmonyOS/LiteOS_m/LiteOS/components/net/test/net_socket_test_004.d \
./example/HarmonyOS/LiteOS_m/LiteOS/components/net/test/net_socket_test_005.d \
./example/HarmonyOS/LiteOS_m/LiteOS/components/net/test/net_socket_test_006.d \
./example/HarmonyOS/LiteOS_m/LiteOS/components/net/test/net_socket_test_007.d \
./example/HarmonyOS/LiteOS_m/LiteOS/components/net/test/net_socket_test_008.d \
./example/HarmonyOS/LiteOS_m/LiteOS/components/net/test/net_socket_test_009.d \
./example/HarmonyOS/LiteOS_m/LiteOS/components/net/test/net_socket_test_010.d \
./example/HarmonyOS/LiteOS_m/LiteOS/components/net/test/net_socket_test_011.d \
./example/HarmonyOS/LiteOS_m/LiteOS/components/net/test/net_socket_test_012.d \
./example/HarmonyOS/LiteOS_m/LiteOS/components/net/test/net_socket_test_013.d \
./example/HarmonyOS/LiteOS_m/LiteOS/components/net/test/test_main.d 

OBJS += \
./example/HarmonyOS/LiteOS_m/LiteOS/components/net/test/net_socket_test_001.o \
./example/HarmonyOS/LiteOS_m/LiteOS/components/net/test/net_socket_test_002.o \
./example/HarmonyOS/LiteOS_m/LiteOS/components/net/test/net_socket_test_003.o \
./example/HarmonyOS/LiteOS_m/LiteOS/components/net/test/net_socket_test_004.o \
./example/HarmonyOS/LiteOS_m/LiteOS/components/net/test/net_socket_test_005.o \
./example/HarmonyOS/LiteOS_m/LiteOS/components/net/test/net_socket_test_006.o \
./example/HarmonyOS/LiteOS_m/LiteOS/components/net/test/net_socket_test_007.o \
./example/HarmonyOS/LiteOS_m/LiteOS/components/net/test/net_socket_test_008.o \
./example/HarmonyOS/LiteOS_m/LiteOS/components/net/test/net_socket_test_009.o \
./example/HarmonyOS/LiteOS_m/LiteOS/components/net/test/net_socket_test_010.o \
./example/HarmonyOS/LiteOS_m/LiteOS/components/net/test/net_socket_test_011.o \
./example/HarmonyOS/LiteOS_m/LiteOS/components/net/test/net_socket_test_012.o \
./example/HarmonyOS/LiteOS_m/LiteOS/components/net/test/net_socket_test_013.o \
./example/HarmonyOS/LiteOS_m/LiteOS/components/net/test/test_main.o 

DIR_OBJS += \
./example/HarmonyOS/LiteOS_m/LiteOS/components/net/test/*.o \

DIR_DEPS += \
./example/HarmonyOS/LiteOS_m/LiteOS/components/net/test/*.d \

DIR_EXPANDS += \
./example/HarmonyOS/LiteOS_m/LiteOS/components/net/test/*.234r.expand \


# Each subdirectory must supply rules for building sources it contributes
example/HarmonyOS/LiteOS_m/LiteOS/components/net/test/%.o: ../example/HarmonyOS/LiteOS_m/LiteOS/components/net/test/%.c
	@	riscv-none-embed-gcc -march=rv32imacxw -mabi=ilp32 -msmall-data-limit=8 -msave-restore -fmax-errors=20 -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -Wuninitialized -g -I"c:/GitProject/CH32V303CBT/Debug" -I"c:/GitProject/CH32V303CBT/Core" -I"c:/GitProject/CH32V303CBT/User" -I"c:/GitProject/CH32V303CBT/Peripheral/inc" -std=gnu99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"

