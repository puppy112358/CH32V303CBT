################################################################################
# MRS Version: 2.4.0
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../example/ETH/Mail/Mail/mail.c \
../example/ETH/Mail/Mail/mailcmd.c 

C_DEPS += \
./example/ETH/Mail/Mail/mail.d \
./example/ETH/Mail/Mail/mailcmd.d 

OBJS += \
./example/ETH/Mail/Mail/mail.o \
./example/ETH/Mail/Mail/mailcmd.o 

DIR_OBJS += \
./example/ETH/Mail/Mail/*.o \

DIR_DEPS += \
./example/ETH/Mail/Mail/*.d \

DIR_EXPANDS += \
./example/ETH/Mail/Mail/*.234r.expand \


# Each subdirectory must supply rules for building sources it contributes
example/ETH/Mail/Mail/%.o: ../example/ETH/Mail/Mail/%.c
	@	riscv-none-embed-gcc -march=rv32imacxw -mabi=ilp32 -msmall-data-limit=8 -msave-restore -fmax-errors=20 -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -Wuninitialized -g -I"c:/GitProject/CH32V303CBT/Debug" -I"c:/GitProject/CH32V303CBT/Core" -I"c:/GitProject/CH32V303CBT/User" -I"c:/GitProject/CH32V303CBT/Peripheral/inc" -std=gnu99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"

