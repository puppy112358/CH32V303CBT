################################################################################
# MRS Version: 2.4.0
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../example/FreeRTOS/FreeRTOS_Core/FreeRTOS/croutine.c \
../example/FreeRTOS/FreeRTOS_Core/FreeRTOS/event_groups.c \
../example/FreeRTOS/FreeRTOS_Core/FreeRTOS/list.c \
../example/FreeRTOS/FreeRTOS_Core/FreeRTOS/queue.c \
../example/FreeRTOS/FreeRTOS_Core/FreeRTOS/stream_buffer.c \
../example/FreeRTOS/FreeRTOS_Core/FreeRTOS/tasks.c \
../example/FreeRTOS/FreeRTOS_Core/FreeRTOS/timers.c 

C_DEPS += \
./example/FreeRTOS/FreeRTOS_Core/FreeRTOS/croutine.d \
./example/FreeRTOS/FreeRTOS_Core/FreeRTOS/event_groups.d \
./example/FreeRTOS/FreeRTOS_Core/FreeRTOS/list.d \
./example/FreeRTOS/FreeRTOS_Core/FreeRTOS/queue.d \
./example/FreeRTOS/FreeRTOS_Core/FreeRTOS/stream_buffer.d \
./example/FreeRTOS/FreeRTOS_Core/FreeRTOS/tasks.d \
./example/FreeRTOS/FreeRTOS_Core/FreeRTOS/timers.d 

OBJS += \
./example/FreeRTOS/FreeRTOS_Core/FreeRTOS/croutine.o \
./example/FreeRTOS/FreeRTOS_Core/FreeRTOS/event_groups.o \
./example/FreeRTOS/FreeRTOS_Core/FreeRTOS/list.o \
./example/FreeRTOS/FreeRTOS_Core/FreeRTOS/queue.o \
./example/FreeRTOS/FreeRTOS_Core/FreeRTOS/stream_buffer.o \
./example/FreeRTOS/FreeRTOS_Core/FreeRTOS/tasks.o \
./example/FreeRTOS/FreeRTOS_Core/FreeRTOS/timers.o 

DIR_OBJS += \
./example/FreeRTOS/FreeRTOS_Core/FreeRTOS/*.o \

DIR_DEPS += \
./example/FreeRTOS/FreeRTOS_Core/FreeRTOS/*.d \

DIR_EXPANDS += \
./example/FreeRTOS/FreeRTOS_Core/FreeRTOS/*.234r.expand \


# Each subdirectory must supply rules for building sources it contributes
example/FreeRTOS/FreeRTOS_Core/FreeRTOS/%.o: ../example/FreeRTOS/FreeRTOS_Core/FreeRTOS/%.c
	@	riscv-none-embed-gcc -march=rv32imacxw -mabi=ilp32 -msmall-data-limit=8 -msave-restore -fmax-errors=20 -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -Wuninitialized -g -I"c:/GitProject/CH32V303CBT/Debug" -I"c:/GitProject/CH32V303CBT/Core" -I"c:/GitProject/CH32V303CBT/User" -I"c:/GitProject/CH32V303CBT/Peripheral/inc" -std=gnu99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"

