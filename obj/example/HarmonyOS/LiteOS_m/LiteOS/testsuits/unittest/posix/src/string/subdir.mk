################################################################################
# MRS Version: 2.4.0
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/posix/src/string/memory_func_test.c \
../example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/posix/src/string/strchr_test.c \
../example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/posix/src/string/string_func_test_01.c \
../example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/posix/src/string/string_func_test_02.c \
../example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/posix/src/string/string_func_test_03.c \
../example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/posix/src/string/strstr_test.c 

C_DEPS += \
./example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/posix/src/string/memory_func_test.d \
./example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/posix/src/string/strchr_test.d \
./example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/posix/src/string/string_func_test_01.d \
./example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/posix/src/string/string_func_test_02.d \
./example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/posix/src/string/string_func_test_03.d \
./example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/posix/src/string/strstr_test.d 

OBJS += \
./example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/posix/src/string/memory_func_test.o \
./example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/posix/src/string/strchr_test.o \
./example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/posix/src/string/string_func_test_01.o \
./example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/posix/src/string/string_func_test_02.o \
./example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/posix/src/string/string_func_test_03.o \
./example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/posix/src/string/strstr_test.o 

DIR_OBJS += \
./example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/posix/src/string/*.o \

DIR_DEPS += \
./example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/posix/src/string/*.d \

DIR_EXPANDS += \
./example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/posix/src/string/*.234r.expand \


# Each subdirectory must supply rules for building sources it contributes
example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/posix/src/string/%.o: ../example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/posix/src/string/%.c
	@	riscv-none-embed-gcc -march=rv32imacxw -mabi=ilp32 -msmall-data-limit=8 -msave-restore -fmax-errors=20 -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -Wuninitialized -g -I"c:/GitProject/CH32V303CBT/Debug" -I"c:/GitProject/CH32V303CBT/Core" -I"c:/GitProject/CH32V303CBT/User" -I"c:/GitProject/CH32V303CBT/Peripheral/inc" -std=gnu99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"

