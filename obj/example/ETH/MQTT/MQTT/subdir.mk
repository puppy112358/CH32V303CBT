################################################################################
# MRS Version: 2.4.0
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../example/ETH/MQTT/MQTT/MQTTConnectClient.c \
../example/ETH/MQTT/MQTT/MQTTDeserializePublish.c \
../example/ETH/MQTT/MQTT/MQTTFormat.c \
../example/ETH/MQTT/MQTT/MQTTPacket.c \
../example/ETH/MQTT/MQTT/MQTTSerializePublish.c \
../example/ETH/MQTT/MQTT/MQTTSubscribeClient.c \
../example/ETH/MQTT/MQTT/MQTTUnsubscribeClient.c 

C_DEPS += \
./example/ETH/MQTT/MQTT/MQTTConnectClient.d \
./example/ETH/MQTT/MQTT/MQTTDeserializePublish.d \
./example/ETH/MQTT/MQTT/MQTTFormat.d \
./example/ETH/MQTT/MQTT/MQTTPacket.d \
./example/ETH/MQTT/MQTT/MQTTSerializePublish.d \
./example/ETH/MQTT/MQTT/MQTTSubscribeClient.d \
./example/ETH/MQTT/MQTT/MQTTUnsubscribeClient.d 

OBJS += \
./example/ETH/MQTT/MQTT/MQTTConnectClient.o \
./example/ETH/MQTT/MQTT/MQTTDeserializePublish.o \
./example/ETH/MQTT/MQTT/MQTTFormat.o \
./example/ETH/MQTT/MQTT/MQTTPacket.o \
./example/ETH/MQTT/MQTT/MQTTSerializePublish.o \
./example/ETH/MQTT/MQTT/MQTTSubscribeClient.o \
./example/ETH/MQTT/MQTT/MQTTUnsubscribeClient.o 

DIR_OBJS += \
./example/ETH/MQTT/MQTT/*.o \

DIR_DEPS += \
./example/ETH/MQTT/MQTT/*.d \

DIR_EXPANDS += \
./example/ETH/MQTT/MQTT/*.234r.expand \


# Each subdirectory must supply rules for building sources it contributes
example/ETH/MQTT/MQTT/%.o: ../example/ETH/MQTT/MQTT/%.c
	@	riscv-none-embed-gcc -march=rv32imacxw -mabi=ilp32 -msmall-data-limit=8 -msave-restore -fmax-errors=20 -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -Wuninitialized -g -I"c:/GitProject/CH32V303CBT/Debug" -I"c:/GitProject/CH32V303CBT/Core" -I"c:/GitProject/CH32V303CBT/User" -I"c:/GitProject/CH32V303CBT/Peripheral/inc" -std=gnu99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"

