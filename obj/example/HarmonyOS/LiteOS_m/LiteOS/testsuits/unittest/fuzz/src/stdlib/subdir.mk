################################################################################
# MRS Version: 2.4.0
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/fuzz/src/stdlib/abs_fuzz.c \
../example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/fuzz/src/stdlib/atoi_fuzz.c \
../example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/fuzz/src/stdlib/atol_fuzz.c \
../example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/fuzz/src/stdlib/atoll_fuzz.c \
../example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/fuzz/src/stdlib/realloc_fuzz.c \
../example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/fuzz/src/stdlib/strtol_fuzz.c \
../example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/fuzz/src/stdlib/strtoul_fuzz.c \
../example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/fuzz/src/stdlib/strtoull_fuzz.c 

C_DEPS += \
./example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/fuzz/src/stdlib/abs_fuzz.d \
./example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/fuzz/src/stdlib/atoi_fuzz.d \
./example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/fuzz/src/stdlib/atol_fuzz.d \
./example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/fuzz/src/stdlib/atoll_fuzz.d \
./example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/fuzz/src/stdlib/realloc_fuzz.d \
./example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/fuzz/src/stdlib/strtol_fuzz.d \
./example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/fuzz/src/stdlib/strtoul_fuzz.d \
./example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/fuzz/src/stdlib/strtoull_fuzz.d 

OBJS += \
./example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/fuzz/src/stdlib/abs_fuzz.o \
./example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/fuzz/src/stdlib/atoi_fuzz.o \
./example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/fuzz/src/stdlib/atol_fuzz.o \
./example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/fuzz/src/stdlib/atoll_fuzz.o \
./example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/fuzz/src/stdlib/realloc_fuzz.o \
./example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/fuzz/src/stdlib/strtol_fuzz.o \
./example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/fuzz/src/stdlib/strtoul_fuzz.o \
./example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/fuzz/src/stdlib/strtoull_fuzz.o 

DIR_OBJS += \
./example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/fuzz/src/stdlib/*.o \

DIR_DEPS += \
./example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/fuzz/src/stdlib/*.d \

DIR_EXPANDS += \
./example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/fuzz/src/stdlib/*.234r.expand \


# Each subdirectory must supply rules for building sources it contributes
example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/fuzz/src/stdlib/%.o: ../example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/fuzz/src/stdlib/%.c
	@	riscv-none-embed-gcc -march=rv32imacxw -mabi=ilp32 -msmall-data-limit=8 -msave-restore -fmax-errors=20 -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -Wuninitialized -g -I"c:/GitProject/CH32V303CBT/Debug" -I"c:/GitProject/CH32V303CBT/Core" -I"c:/GitProject/CH32V303CBT/User" -I"c:/GitProject/CH32V303CBT/Peripheral/inc" -std=gnu99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"

