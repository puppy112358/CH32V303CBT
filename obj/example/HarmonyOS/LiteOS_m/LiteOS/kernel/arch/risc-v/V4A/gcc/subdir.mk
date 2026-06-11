################################################################################
# MRS Version: 2.4.0
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../example/HarmonyOS/LiteOS_m/LiteOS/kernel/arch/risc-v/V4A/gcc/los_context.c \
../example/HarmonyOS/LiteOS_m/LiteOS/kernel/arch/risc-v/V4A/gcc/los_interrupt.c \
../example/HarmonyOS/LiteOS_m/LiteOS/kernel/arch/risc-v/V4A/gcc/los_timer.c 

C_DEPS += \
./example/HarmonyOS/LiteOS_m/LiteOS/kernel/arch/risc-v/V4A/gcc/los_context.d \
./example/HarmonyOS/LiteOS_m/LiteOS/kernel/arch/risc-v/V4A/gcc/los_interrupt.d \
./example/HarmonyOS/LiteOS_m/LiteOS/kernel/arch/risc-v/V4A/gcc/los_timer.d 

S_UPPER_SRCS += \
../example/HarmonyOS/LiteOS_m/LiteOS/kernel/arch/risc-v/V4A/gcc/los_dispatch.S \
../example/HarmonyOS/LiteOS_m/LiteOS/kernel/arch/risc-v/V4A/gcc/los_exc.S 

S_UPPER_DEPS += \
./example/HarmonyOS/LiteOS_m/LiteOS/kernel/arch/risc-v/V4A/gcc/los_dispatch.d \
./example/HarmonyOS/LiteOS_m/LiteOS/kernel/arch/risc-v/V4A/gcc/los_exc.d 

OBJS += \
./example/HarmonyOS/LiteOS_m/LiteOS/kernel/arch/risc-v/V4A/gcc/los_context.o \
./example/HarmonyOS/LiteOS_m/LiteOS/kernel/arch/risc-v/V4A/gcc/los_dispatch.o \
./example/HarmonyOS/LiteOS_m/LiteOS/kernel/arch/risc-v/V4A/gcc/los_exc.o \
./example/HarmonyOS/LiteOS_m/LiteOS/kernel/arch/risc-v/V4A/gcc/los_interrupt.o \
./example/HarmonyOS/LiteOS_m/LiteOS/kernel/arch/risc-v/V4A/gcc/los_timer.o 

DIR_OBJS += \
./example/HarmonyOS/LiteOS_m/LiteOS/kernel/arch/risc-v/V4A/gcc/*.o \

DIR_DEPS += \
./example/HarmonyOS/LiteOS_m/LiteOS/kernel/arch/risc-v/V4A/gcc/*.d \

DIR_EXPANDS += \
./example/HarmonyOS/LiteOS_m/LiteOS/kernel/arch/risc-v/V4A/gcc/*.234r.expand \


# Each subdirectory must supply rules for building sources it contributes
example/HarmonyOS/LiteOS_m/LiteOS/kernel/arch/risc-v/V4A/gcc/%.o: ../example/HarmonyOS/LiteOS_m/LiteOS/kernel/arch/risc-v/V4A/gcc/%.c
	@	riscv-none-embed-gcc -march=rv32imacxw -mabi=ilp32 -msmall-data-limit=8 -msave-restore -fmax-errors=20 -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -Wuninitialized -g -I"c:/GitProject/CH32V303CBT/Debug" -I"c:/GitProject/CH32V303CBT/Core" -I"c:/GitProject/CH32V303CBT/User" -I"c:/GitProject/CH32V303CBT/Peripheral/inc" -std=gnu99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"

example/HarmonyOS/LiteOS_m/LiteOS/kernel/arch/risc-v/V4A/gcc/%.o: ../example/HarmonyOS/LiteOS_m/LiteOS/kernel/arch/risc-v/V4A/gcc/%.S
	@	riscv-none-embed-gcc -march=rv32imacxw -mabi=ilp32 -msmall-data-limit=8 -msave-restore -fmax-errors=20 -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -Wuninitialized -g -x assembler-with-cpp -I"c:/GitProject/CH32V303CBT/Startup" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"

