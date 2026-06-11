################################################################################
# MRS Version: 2.4.0
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/fuzz/src/strings/it_test_strings_fuzz.c \
../example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/fuzz/src/strings/test_strncasecmp_fuzz.c 

C_DEPS += \
./example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/fuzz/src/strings/it_test_strings_fuzz.d \
./example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/fuzz/src/strings/test_strncasecmp_fuzz.d 

OBJS += \
./example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/fuzz/src/strings/it_test_strings_fuzz.o \
./example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/fuzz/src/strings/test_strncasecmp_fuzz.o 

DIR_OBJS += \
./example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/fuzz/src/strings/*.o \

DIR_DEPS += \
./example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/fuzz/src/strings/*.d \

DIR_EXPANDS += \
./example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/fuzz/src/strings/*.234r.expand \


# Each subdirectory must supply rules for building sources it contributes
example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/fuzz/src/strings/%.o: ../example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/fuzz/src/strings/%.c
	@	riscv-none-embed-gcc -march=rv32imacxw -mabi=ilp32 -msmall-data-limit=8 -msave-restore -fmax-errors=20 -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -Wuninitialized -g -I"c:/GitProject/CH32V303CBT/Debug" -I"c:/GitProject/CH32V303CBT/Core" -I"c:/GitProject/CH32V303CBT/User" -I"c:/GitProject/CH32V303CBT/Peripheral/inc" -std=gnu99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"

