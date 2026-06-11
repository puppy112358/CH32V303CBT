################################################################################
# MRS Version: 2.4.0
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../example/RT-Thread/rt-thread/rtthread/libcpu/risc-v/common/cpuport.c 

C_DEPS += \
./example/RT-Thread/rt-thread/rtthread/libcpu/risc-v/common/cpuport.d 

S_UPPER_SRCS += \
../example/RT-Thread/rt-thread/rtthread/libcpu/risc-v/common/context_gcc.S \
../example/RT-Thread/rt-thread/rtthread/libcpu/risc-v/common/interrupt_gcc.S 

S_UPPER_DEPS += \
./example/RT-Thread/rt-thread/rtthread/libcpu/risc-v/common/context_gcc.d \
./example/RT-Thread/rt-thread/rtthread/libcpu/risc-v/common/interrupt_gcc.d 

OBJS += \
./example/RT-Thread/rt-thread/rtthread/libcpu/risc-v/common/context_gcc.o \
./example/RT-Thread/rt-thread/rtthread/libcpu/risc-v/common/cpuport.o \
./example/RT-Thread/rt-thread/rtthread/libcpu/risc-v/common/interrupt_gcc.o 

DIR_OBJS += \
./example/RT-Thread/rt-thread/rtthread/libcpu/risc-v/common/*.o \

DIR_DEPS += \
./example/RT-Thread/rt-thread/rtthread/libcpu/risc-v/common/*.d \

DIR_EXPANDS += \
./example/RT-Thread/rt-thread/rtthread/libcpu/risc-v/common/*.234r.expand \


# Each subdirectory must supply rules for building sources it contributes
example/RT-Thread/rt-thread/rtthread/libcpu/risc-v/common/%.o: ../example/RT-Thread/rt-thread/rtthread/libcpu/risc-v/common/%.c
	@	riscv-none-embed-gcc -march=rv32imacxw -mabi=ilp32 -msmall-data-limit=8 -msave-restore -fmax-errors=20 -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -Wuninitialized -g -I"c:/GitProject/CH32V303CBT/Debug" -I"c:/GitProject/CH32V303CBT/Core" -I"c:/GitProject/CH32V303CBT/User" -I"c:/GitProject/CH32V303CBT/Peripheral/inc" -std=gnu99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"

example/RT-Thread/rt-thread/rtthread/libcpu/risc-v/common/%.o: ../example/RT-Thread/rt-thread/rtthread/libcpu/risc-v/common/%.S
	@	riscv-none-embed-gcc -march=rv32imacxw -mabi=ilp32 -msmall-data-limit=8 -msave-restore -fmax-errors=20 -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -Wuninitialized -g -x assembler-with-cpp -I"c:/GitProject/CH32V303CBT/Startup" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"

