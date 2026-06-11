################################################################################
# MRS Version: 2.4.0
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../example/DVP/DVP_TFTLCD/User/ch32v30x_it.c \
../example/DVP/DVP_TFTLCD/User/lcd.c \
../example/DVP/DVP_TFTLCD/User/main.c \
../example/DVP/DVP_TFTLCD/User/ov.c \
../example/DVP/DVP_TFTLCD/User/system_ch32v30x.c 

C_DEPS += \
./example/DVP/DVP_TFTLCD/User/ch32v30x_it.d \
./example/DVP/DVP_TFTLCD/User/lcd.d \
./example/DVP/DVP_TFTLCD/User/main.d \
./example/DVP/DVP_TFTLCD/User/ov.d \
./example/DVP/DVP_TFTLCD/User/system_ch32v30x.d 

OBJS += \
./example/DVP/DVP_TFTLCD/User/ch32v30x_it.o \
./example/DVP/DVP_TFTLCD/User/lcd.o \
./example/DVP/DVP_TFTLCD/User/main.o \
./example/DVP/DVP_TFTLCD/User/ov.o \
./example/DVP/DVP_TFTLCD/User/system_ch32v30x.o 

DIR_OBJS += \
./example/DVP/DVP_TFTLCD/User/*.o \

DIR_DEPS += \
./example/DVP/DVP_TFTLCD/User/*.d \

DIR_EXPANDS += \
./example/DVP/DVP_TFTLCD/User/*.234r.expand \


# Each subdirectory must supply rules for building sources it contributes
example/DVP/DVP_TFTLCD/User/%.o: ../example/DVP/DVP_TFTLCD/User/%.c
	@	riscv-none-embed-gcc -march=rv32imacxw -mabi=ilp32 -msmall-data-limit=8 -msave-restore -fmax-errors=20 -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -Wuninitialized -g -I"c:/GitProject/CH32V303CBT/Debug" -I"c:/GitProject/CH32V303CBT/Core" -I"c:/GitProject/CH32V303CBT/User" -I"c:/GitProject/CH32V303CBT/Peripheral/inc" -std=gnu99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"

