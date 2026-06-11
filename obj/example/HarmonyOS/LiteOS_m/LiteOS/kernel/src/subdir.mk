################################################################################
# MRS Version: 2.4.0
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../example/HarmonyOS/LiteOS_m/LiteOS/kernel/src/los_event.c \
../example/HarmonyOS/LiteOS_m/LiteOS/kernel/src/los_init.c \
../example/HarmonyOS/LiteOS_m/LiteOS/kernel/src/los_mux.c \
../example/HarmonyOS/LiteOS_m/LiteOS/kernel/src/los_queue.c \
../example/HarmonyOS/LiteOS_m/LiteOS/kernel/src/los_sched.c \
../example/HarmonyOS/LiteOS_m/LiteOS/kernel/src/los_sem.c \
../example/HarmonyOS/LiteOS_m/LiteOS/kernel/src/los_sortlink.c \
../example/HarmonyOS/LiteOS_m/LiteOS/kernel/src/los_swtmr.c \
../example/HarmonyOS/LiteOS_m/LiteOS/kernel/src/los_task.c \
../example/HarmonyOS/LiteOS_m/LiteOS/kernel/src/los_tick.c 

C_DEPS += \
./example/HarmonyOS/LiteOS_m/LiteOS/kernel/src/los_event.d \
./example/HarmonyOS/LiteOS_m/LiteOS/kernel/src/los_init.d \
./example/HarmonyOS/LiteOS_m/LiteOS/kernel/src/los_mux.d \
./example/HarmonyOS/LiteOS_m/LiteOS/kernel/src/los_queue.d \
./example/HarmonyOS/LiteOS_m/LiteOS/kernel/src/los_sched.d \
./example/HarmonyOS/LiteOS_m/LiteOS/kernel/src/los_sem.d \
./example/HarmonyOS/LiteOS_m/LiteOS/kernel/src/los_sortlink.d \
./example/HarmonyOS/LiteOS_m/LiteOS/kernel/src/los_swtmr.d \
./example/HarmonyOS/LiteOS_m/LiteOS/kernel/src/los_task.d \
./example/HarmonyOS/LiteOS_m/LiteOS/kernel/src/los_tick.d 

OBJS += \
./example/HarmonyOS/LiteOS_m/LiteOS/kernel/src/los_event.o \
./example/HarmonyOS/LiteOS_m/LiteOS/kernel/src/los_init.o \
./example/HarmonyOS/LiteOS_m/LiteOS/kernel/src/los_mux.o \
./example/HarmonyOS/LiteOS_m/LiteOS/kernel/src/los_queue.o \
./example/HarmonyOS/LiteOS_m/LiteOS/kernel/src/los_sched.o \
./example/HarmonyOS/LiteOS_m/LiteOS/kernel/src/los_sem.o \
./example/HarmonyOS/LiteOS_m/LiteOS/kernel/src/los_sortlink.o \
./example/HarmonyOS/LiteOS_m/LiteOS/kernel/src/los_swtmr.o \
./example/HarmonyOS/LiteOS_m/LiteOS/kernel/src/los_task.o \
./example/HarmonyOS/LiteOS_m/LiteOS/kernel/src/los_tick.o 

DIR_OBJS += \
./example/HarmonyOS/LiteOS_m/LiteOS/kernel/src/*.o \

DIR_DEPS += \
./example/HarmonyOS/LiteOS_m/LiteOS/kernel/src/*.d \

DIR_EXPANDS += \
./example/HarmonyOS/LiteOS_m/LiteOS/kernel/src/*.234r.expand \


# Each subdirectory must supply rules for building sources it contributes
example/HarmonyOS/LiteOS_m/LiteOS/kernel/src/%.o: ../example/HarmonyOS/LiteOS_m/LiteOS/kernel/src/%.c
	@	riscv-none-embed-gcc -march=rv32imacxw -mabi=ilp32 -msmall-data-limit=8 -msave-restore -fmax-errors=20 -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -Wuninitialized -g -I"c:/GitProject/CH32V303CBT/Debug" -I"c:/GitProject/CH32V303CBT/Core" -I"c:/GitProject/CH32V303CBT/User" -I"c:/GitProject/CH32V303CBT/Peripheral/inc" -std=gnu99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"

