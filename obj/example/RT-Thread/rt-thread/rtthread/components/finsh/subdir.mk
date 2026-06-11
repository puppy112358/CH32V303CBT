################################################################################
# MRS Version: 2.4.0
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../example/RT-Thread/rt-thread/rtthread/components/finsh/cmd.c \
../example/RT-Thread/rt-thread/rtthread/components/finsh/msh.c \
../example/RT-Thread/rt-thread/rtthread/components/finsh/msh_cmd.c \
../example/RT-Thread/rt-thread/rtthread/components/finsh/msh_file.c \
../example/RT-Thread/rt-thread/rtthread/components/finsh/shell.c \
../example/RT-Thread/rt-thread/rtthread/components/finsh/symbol.c 

C_DEPS += \
./example/RT-Thread/rt-thread/rtthread/components/finsh/cmd.d \
./example/RT-Thread/rt-thread/rtthread/components/finsh/msh.d \
./example/RT-Thread/rt-thread/rtthread/components/finsh/msh_cmd.d \
./example/RT-Thread/rt-thread/rtthread/components/finsh/msh_file.d \
./example/RT-Thread/rt-thread/rtthread/components/finsh/shell.d \
./example/RT-Thread/rt-thread/rtthread/components/finsh/symbol.d 

OBJS += \
./example/RT-Thread/rt-thread/rtthread/components/finsh/cmd.o \
./example/RT-Thread/rt-thread/rtthread/components/finsh/msh.o \
./example/RT-Thread/rt-thread/rtthread/components/finsh/msh_cmd.o \
./example/RT-Thread/rt-thread/rtthread/components/finsh/msh_file.o \
./example/RT-Thread/rt-thread/rtthread/components/finsh/shell.o \
./example/RT-Thread/rt-thread/rtthread/components/finsh/symbol.o 

DIR_OBJS += \
./example/RT-Thread/rt-thread/rtthread/components/finsh/*.o \

DIR_DEPS += \
./example/RT-Thread/rt-thread/rtthread/components/finsh/*.d \

DIR_EXPANDS += \
./example/RT-Thread/rt-thread/rtthread/components/finsh/*.234r.expand \


# Each subdirectory must supply rules for building sources it contributes
example/RT-Thread/rt-thread/rtthread/components/finsh/%.o: ../example/RT-Thread/rt-thread/rtthread/components/finsh/%.c
	@	riscv-none-embed-gcc -march=rv32imacxw -mabi=ilp32 -msmall-data-limit=8 -msave-restore -fmax-errors=20 -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -Wuninitialized -g -I"c:/GitProject/CH32V303CBT/Debug" -I"c:/GitProject/CH32V303CBT/Core" -I"c:/GitProject/CH32V303CBT/User" -I"c:/GitProject/CH32V303CBT/Peripheral/inc" -std=gnu99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"

