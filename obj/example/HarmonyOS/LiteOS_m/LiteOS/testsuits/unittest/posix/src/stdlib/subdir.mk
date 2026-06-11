################################################################################
# MRS Version: 2.4.0
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/posix/src/stdlib/atoi_test.c \
../example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/posix/src/stdlib/atol_test.c \
../example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/posix/src/stdlib/atoll_test.c \
../example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/posix/src/stdlib/strtol_test.c \
../example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/posix/src/stdlib/strtoul_test.c \
../example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/posix/src/stdlib/strtoull_test.c 

C_DEPS += \
./example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/posix/src/stdlib/atoi_test.d \
./example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/posix/src/stdlib/atol_test.d \
./example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/posix/src/stdlib/atoll_test.d \
./example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/posix/src/stdlib/strtol_test.d \
./example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/posix/src/stdlib/strtoul_test.d \
./example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/posix/src/stdlib/strtoull_test.d 

OBJS += \
./example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/posix/src/stdlib/atoi_test.o \
./example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/posix/src/stdlib/atol_test.o \
./example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/posix/src/stdlib/atoll_test.o \
./example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/posix/src/stdlib/strtol_test.o \
./example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/posix/src/stdlib/strtoul_test.o \
./example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/posix/src/stdlib/strtoull_test.o 

DIR_OBJS += \
./example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/posix/src/stdlib/*.o \

DIR_DEPS += \
./example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/posix/src/stdlib/*.d \

DIR_EXPANDS += \
./example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/posix/src/stdlib/*.234r.expand \


# Each subdirectory must supply rules for building sources it contributes
example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/posix/src/stdlib/%.o: ../example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/posix/src/stdlib/%.c
	@	riscv-none-embed-gcc -march=rv32imacxw -mabi=ilp32 -msmall-data-limit=8 -msave-restore -fmax-errors=20 -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -Wuninitialized -g -I"c:/GitProject/CH32V303CBT/Debug" -I"c:/GitProject/CH32V303CBT/Core" -I"c:/GitProject/CH32V303CBT/User" -I"c:/GitProject/CH32V303CBT/Peripheral/inc" -std=gnu99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"

