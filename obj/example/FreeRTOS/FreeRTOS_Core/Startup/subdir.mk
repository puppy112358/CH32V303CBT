################################################################################
# MRS Version: 2.4.0
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
S_UPPER_SRCS += \
../example/FreeRTOS/FreeRTOS_Core/Startup/startup_ch32v30x_D8.S \
../example/FreeRTOS/FreeRTOS_Core/Startup/startup_ch32v30x_D8C.S 

S_UPPER_DEPS += \
./example/FreeRTOS/FreeRTOS_Core/Startup/startup_ch32v30x_D8.d \
./example/FreeRTOS/FreeRTOS_Core/Startup/startup_ch32v30x_D8C.d 

OBJS += \
./example/FreeRTOS/FreeRTOS_Core/Startup/startup_ch32v30x_D8.o \
./example/FreeRTOS/FreeRTOS_Core/Startup/startup_ch32v30x_D8C.o 

DIR_OBJS += \
./example/FreeRTOS/FreeRTOS_Core/Startup/*.o \

DIR_DEPS += \
./example/FreeRTOS/FreeRTOS_Core/Startup/*.d \

DIR_EXPANDS += \
./example/FreeRTOS/FreeRTOS_Core/Startup/*.234r.expand \


# Each subdirectory must supply rules for building sources it contributes
example/FreeRTOS/FreeRTOS_Core/Startup/%.o: ../example/FreeRTOS/FreeRTOS_Core/Startup/%.S
	@	riscv-none-embed-gcc -march=rv32imacxw -mabi=ilp32 -msmall-data-limit=8 -msave-restore -fmax-errors=20 -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -Wuninitialized -g -x assembler-with-cpp -I"c:/GitProject/CH32V303CBT/Startup" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"

