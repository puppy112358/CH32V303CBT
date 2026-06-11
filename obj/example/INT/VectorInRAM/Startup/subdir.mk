################################################################################
# MRS Version: 2.4.0
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
S_UPPER_SRCS += \
../example/INT/VectorInRAM/Startup/startup_ch32v30x_D8C_vector.S \
../example/INT/VectorInRAM/Startup/startup_ch32v30x_D8_vector.S 

S_UPPER_DEPS += \
./example/INT/VectorInRAM/Startup/startup_ch32v30x_D8C_vector.d \
./example/INT/VectorInRAM/Startup/startup_ch32v30x_D8_vector.d 

OBJS += \
./example/INT/VectorInRAM/Startup/startup_ch32v30x_D8C_vector.o \
./example/INT/VectorInRAM/Startup/startup_ch32v30x_D8_vector.o 

DIR_OBJS += \
./example/INT/VectorInRAM/Startup/*.o \

DIR_DEPS += \
./example/INT/VectorInRAM/Startup/*.d \

DIR_EXPANDS += \
./example/INT/VectorInRAM/Startup/*.234r.expand \


# Each subdirectory must supply rules for building sources it contributes
example/INT/VectorInRAM/Startup/%.o: ../example/INT/VectorInRAM/Startup/%.S
	@	riscv-none-embed-gcc -march=rv32imacxw -mabi=ilp32 -msmall-data-limit=8 -msave-restore -fmax-errors=20 -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -Wuninitialized -g -x assembler-with-cpp -I"c:/GitProject/CH32V303CBT/Startup" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"

