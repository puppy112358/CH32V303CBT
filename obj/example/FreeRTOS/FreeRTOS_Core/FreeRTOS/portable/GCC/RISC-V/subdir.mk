################################################################################
# MRS Version: 2.4.0
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../example/FreeRTOS/FreeRTOS_Core/FreeRTOS/portable/GCC/RISC-V/port.c 

C_DEPS += \
./example/FreeRTOS/FreeRTOS_Core/FreeRTOS/portable/GCC/RISC-V/port.d 

S_UPPER_SRCS += \
../example/FreeRTOS/FreeRTOS_Core/FreeRTOS/portable/GCC/RISC-V/portASM.S 

S_UPPER_DEPS += \
./example/FreeRTOS/FreeRTOS_Core/FreeRTOS/portable/GCC/RISC-V/portASM.d 

OBJS += \
./example/FreeRTOS/FreeRTOS_Core/FreeRTOS/portable/GCC/RISC-V/port.o \
./example/FreeRTOS/FreeRTOS_Core/FreeRTOS/portable/GCC/RISC-V/portASM.o 

DIR_OBJS += \
./example/FreeRTOS/FreeRTOS_Core/FreeRTOS/portable/GCC/RISC-V/*.o \

DIR_DEPS += \
./example/FreeRTOS/FreeRTOS_Core/FreeRTOS/portable/GCC/RISC-V/*.d \

DIR_EXPANDS += \
./example/FreeRTOS/FreeRTOS_Core/FreeRTOS/portable/GCC/RISC-V/*.234r.expand \


# Each subdirectory must supply rules for building sources it contributes
example/FreeRTOS/FreeRTOS_Core/FreeRTOS/portable/GCC/RISC-V/%.o: ../example/FreeRTOS/FreeRTOS_Core/FreeRTOS/portable/GCC/RISC-V/%.c
	@	riscv-none-embed-gcc -march=rv32imacxw -mabi=ilp32 -msmall-data-limit=8 -msave-restore -fmax-errors=20 -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -Wuninitialized -g -I"c:/GitProject/CH32V303CBT/Debug" -I"c:/GitProject/CH32V303CBT/Core" -I"c:/GitProject/CH32V303CBT/User" -I"c:/GitProject/CH32V303CBT/Peripheral/inc" -std=gnu99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"

example/FreeRTOS/FreeRTOS_Core/FreeRTOS/portable/GCC/RISC-V/%.o: ../example/FreeRTOS/FreeRTOS_Core/FreeRTOS/portable/GCC/RISC-V/%.S
	@	riscv-none-embed-gcc -march=rv32imacxw -mabi=ilp32 -msmall-data-limit=8 -msave-restore -fmax-errors=20 -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -Wuninitialized -g -x assembler-with-cpp -I"c:/GitProject/CH32V303CBT/Startup" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"

