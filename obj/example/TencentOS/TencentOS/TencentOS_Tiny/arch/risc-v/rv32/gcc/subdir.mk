################################################################################
# MRS Version: 2.4.0
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../example/TencentOS/TencentOS/TencentOS_Tiny/arch/risc-v/rv32/gcc/port_c.c 

C_DEPS += \
./example/TencentOS/TencentOS/TencentOS_Tiny/arch/risc-v/rv32/gcc/port_c.d 

S_UPPER_SRCS += \
../example/TencentOS/TencentOS/TencentOS_Tiny/arch/risc-v/rv32/gcc/port_s.S 

S_UPPER_DEPS += \
./example/TencentOS/TencentOS/TencentOS_Tiny/arch/risc-v/rv32/gcc/port_s.d 

OBJS += \
./example/TencentOS/TencentOS/TencentOS_Tiny/arch/risc-v/rv32/gcc/port_c.o \
./example/TencentOS/TencentOS/TencentOS_Tiny/arch/risc-v/rv32/gcc/port_s.o 

DIR_OBJS += \
./example/TencentOS/TencentOS/TencentOS_Tiny/arch/risc-v/rv32/gcc/*.o \

DIR_DEPS += \
./example/TencentOS/TencentOS/TencentOS_Tiny/arch/risc-v/rv32/gcc/*.d \

DIR_EXPANDS += \
./example/TencentOS/TencentOS/TencentOS_Tiny/arch/risc-v/rv32/gcc/*.234r.expand \


# Each subdirectory must supply rules for building sources it contributes
example/TencentOS/TencentOS/TencentOS_Tiny/arch/risc-v/rv32/gcc/%.o: ../example/TencentOS/TencentOS/TencentOS_Tiny/arch/risc-v/rv32/gcc/%.c
	@	riscv-none-embed-gcc -march=rv32imacxw -mabi=ilp32 -msmall-data-limit=8 -msave-restore -fmax-errors=20 -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -Wuninitialized -g -I"c:/GitProject/CH32V303CBT/Debug" -I"c:/GitProject/CH32V303CBT/Core" -I"c:/GitProject/CH32V303CBT/User" -I"c:/GitProject/CH32V303CBT/Peripheral/inc" -std=gnu99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"

example/TencentOS/TencentOS/TencentOS_Tiny/arch/risc-v/rv32/gcc/%.o: ../example/TencentOS/TencentOS/TencentOS_Tiny/arch/risc-v/rv32/gcc/%.S
	@	riscv-none-embed-gcc -march=rv32imacxw -mabi=ilp32 -msmall-data-limit=8 -msave-restore -fmax-errors=20 -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -Wuninitialized -g -x assembler-with-cpp -I"c:/GitProject/CH32V303CBT/Startup" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"

