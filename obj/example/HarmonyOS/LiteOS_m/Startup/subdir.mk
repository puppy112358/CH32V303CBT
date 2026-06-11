################################################################################
# MRS Version: 2.4.0
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
S_UPPER_SRCS += \
../example/HarmonyOS/LiteOS_m/Startup/startup_ch32v30x_D8.S \
../example/HarmonyOS/LiteOS_m/Startup/startup_ch32v30x_D8C.S 

S_UPPER_DEPS += \
./example/HarmonyOS/LiteOS_m/Startup/startup_ch32v30x_D8.d \
./example/HarmonyOS/LiteOS_m/Startup/startup_ch32v30x_D8C.d 

OBJS += \
./example/HarmonyOS/LiteOS_m/Startup/startup_ch32v30x_D8.o \
./example/HarmonyOS/LiteOS_m/Startup/startup_ch32v30x_D8C.o 

DIR_OBJS += \
./example/HarmonyOS/LiteOS_m/Startup/*.o \

DIR_DEPS += \
./example/HarmonyOS/LiteOS_m/Startup/*.d \

DIR_EXPANDS += \
./example/HarmonyOS/LiteOS_m/Startup/*.234r.expand \


# Each subdirectory must supply rules for building sources it contributes
example/HarmonyOS/LiteOS_m/Startup/%.o: ../example/HarmonyOS/LiteOS_m/Startup/%.S
	@	riscv-none-embed-gcc -march=rv32imacxw -mabi=ilp32 -msmall-data-limit=8 -msave-restore -fmax-errors=20 -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -Wuninitialized -g -x assembler-with-cpp -I"c:/GitProject/CH32V303CBT/Startup" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"

