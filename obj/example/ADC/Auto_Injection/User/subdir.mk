################################################################################
# MRS Version: 2.4.0
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../example/ADC/Auto_Injection/User/ch32v30x_it.c \
../example/ADC/Auto_Injection/User/main.c \
../example/ADC/Auto_Injection/User/system_ch32v30x.c 

C_DEPS += \
./example/ADC/Auto_Injection/User/ch32v30x_it.d \
./example/ADC/Auto_Injection/User/main.d \
./example/ADC/Auto_Injection/User/system_ch32v30x.d 

OBJS += \
./example/ADC/Auto_Injection/User/ch32v30x_it.o \
./example/ADC/Auto_Injection/User/main.o \
./example/ADC/Auto_Injection/User/system_ch32v30x.o 

DIR_OBJS += \
./example/ADC/Auto_Injection/User/*.o \

DIR_DEPS += \
./example/ADC/Auto_Injection/User/*.d \

DIR_EXPANDS += \
./example/ADC/Auto_Injection/User/*.234r.expand \


# Each subdirectory must supply rules for building sources it contributes
example/ADC/Auto_Injection/User/%.o: ../example/ADC/Auto_Injection/User/%.c
	@	riscv-none-embed-gcc -march=rv32imacxw -mabi=ilp32 -msmall-data-limit=8 -msave-restore -fmax-errors=20 -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -Wuninitialized -g -I"c:/GitProject/CH32V303CBT/Debug" -I"c:/GitProject/CH32V303CBT/Core" -I"c:/GitProject/CH32V303CBT/User" -I"c:/GitProject/CH32V303CBT/Peripheral/inc" -std=gnu99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"

