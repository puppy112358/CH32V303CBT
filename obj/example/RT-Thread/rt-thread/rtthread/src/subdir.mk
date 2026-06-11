################################################################################
# MRS Version: 2.4.0
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../example/RT-Thread/rt-thread/rtthread/src/clock.c \
../example/RT-Thread/rt-thread/rtthread/src/components.c \
../example/RT-Thread/rt-thread/rtthread/src/cpu.c \
../example/RT-Thread/rt-thread/rtthread/src/device.c \
../example/RT-Thread/rt-thread/rtthread/src/idle.c \
../example/RT-Thread/rt-thread/rtthread/src/ipc.c \
../example/RT-Thread/rt-thread/rtthread/src/irq.c \
../example/RT-Thread/rt-thread/rtthread/src/kservice.c \
../example/RT-Thread/rt-thread/rtthread/src/mem.c \
../example/RT-Thread/rt-thread/rtthread/src/memheap.c \
../example/RT-Thread/rt-thread/rtthread/src/mempool.c \
../example/RT-Thread/rt-thread/rtthread/src/object.c \
../example/RT-Thread/rt-thread/rtthread/src/scheduler.c \
../example/RT-Thread/rt-thread/rtthread/src/slab.c \
../example/RT-Thread/rt-thread/rtthread/src/thread.c \
../example/RT-Thread/rt-thread/rtthread/src/timer.c 

C_DEPS += \
./example/RT-Thread/rt-thread/rtthread/src/clock.d \
./example/RT-Thread/rt-thread/rtthread/src/components.d \
./example/RT-Thread/rt-thread/rtthread/src/cpu.d \
./example/RT-Thread/rt-thread/rtthread/src/device.d \
./example/RT-Thread/rt-thread/rtthread/src/idle.d \
./example/RT-Thread/rt-thread/rtthread/src/ipc.d \
./example/RT-Thread/rt-thread/rtthread/src/irq.d \
./example/RT-Thread/rt-thread/rtthread/src/kservice.d \
./example/RT-Thread/rt-thread/rtthread/src/mem.d \
./example/RT-Thread/rt-thread/rtthread/src/memheap.d \
./example/RT-Thread/rt-thread/rtthread/src/mempool.d \
./example/RT-Thread/rt-thread/rtthread/src/object.d \
./example/RT-Thread/rt-thread/rtthread/src/scheduler.d \
./example/RT-Thread/rt-thread/rtthread/src/slab.d \
./example/RT-Thread/rt-thread/rtthread/src/thread.d \
./example/RT-Thread/rt-thread/rtthread/src/timer.d 

OBJS += \
./example/RT-Thread/rt-thread/rtthread/src/clock.o \
./example/RT-Thread/rt-thread/rtthread/src/components.o \
./example/RT-Thread/rt-thread/rtthread/src/cpu.o \
./example/RT-Thread/rt-thread/rtthread/src/device.o \
./example/RT-Thread/rt-thread/rtthread/src/idle.o \
./example/RT-Thread/rt-thread/rtthread/src/ipc.o \
./example/RT-Thread/rt-thread/rtthread/src/irq.o \
./example/RT-Thread/rt-thread/rtthread/src/kservice.o \
./example/RT-Thread/rt-thread/rtthread/src/mem.o \
./example/RT-Thread/rt-thread/rtthread/src/memheap.o \
./example/RT-Thread/rt-thread/rtthread/src/mempool.o \
./example/RT-Thread/rt-thread/rtthread/src/object.o \
./example/RT-Thread/rt-thread/rtthread/src/scheduler.o \
./example/RT-Thread/rt-thread/rtthread/src/slab.o \
./example/RT-Thread/rt-thread/rtthread/src/thread.o \
./example/RT-Thread/rt-thread/rtthread/src/timer.o 

DIR_OBJS += \
./example/RT-Thread/rt-thread/rtthread/src/*.o \

DIR_DEPS += \
./example/RT-Thread/rt-thread/rtthread/src/*.d \

DIR_EXPANDS += \
./example/RT-Thread/rt-thread/rtthread/src/*.234r.expand \


# Each subdirectory must supply rules for building sources it contributes
example/RT-Thread/rt-thread/rtthread/src/%.o: ../example/RT-Thread/rt-thread/rtthread/src/%.c
	@	riscv-none-embed-gcc -march=rv32imacxw -mabi=ilp32 -msmall-data-limit=8 -msave-restore -fmax-errors=20 -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -Wuninitialized -g -I"c:/GitProject/CH32V303CBT/Debug" -I"c:/GitProject/CH32V303CBT/Core" -I"c:/GitProject/CH32V303CBT/User" -I"c:/GitProject/CH32V303CBT/Peripheral/inc" -std=gnu99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"

