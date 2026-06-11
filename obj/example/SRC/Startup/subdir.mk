################################################################################
# MRS Version: 2.4.0
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
S_UPPER_SRCS += \
../example/SRC/Startup/startup_ch32v30x_D8.S \
../example/SRC/Startup/startup_ch32v30x_D8C.S 

S_UPPER_DEPS += \
./example/SRC/Startup/startup_ch32v30x_D8.d \
./example/SRC/Startup/startup_ch32v30x_D8C.d 

OBJS += \
./example/SRC/Startup/startup_ch32v30x_D8.o \
./example/SRC/Startup/startup_ch32v30x_D8C.o 

DIR_OBJS += \
./example/SRC/Startup/*.o \

DIR_DEPS += \
./example/SRC/Startup/*.d \

DIR_EXPANDS += \
./example/SRC/Startup/*.234r.expand \


# Each subdirectory must supply rules for building sources it contributes
example/SRC/Startup/%.o: ../example/SRC/Startup/%.S
	@	riscv-none-embed-gcc -march=rv32imacxw -mabi=ilp32 -msmall-data-limit=8 -msave-restore -fmax-errors=20 -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -Wuninitialized -g -x assembler-with-cpp -I"c:/GitProject/CH32V303CBT/Startup" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"

