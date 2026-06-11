################################################################################
# MRS Version: 2.4.0
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/posix/src/ctype/ctype_func_test.c \
../example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/posix/src/ctype/isdigit_test.c \
../example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/posix/src/ctype/islower_test.c \
../example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/posix/src/ctype/isxdigit_test.c \
../example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/posix/src/ctype/tolower_test.c \
../example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/posix/src/ctype/toupper_test.c 

C_DEPS += \
./example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/posix/src/ctype/ctype_func_test.d \
./example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/posix/src/ctype/isdigit_test.d \
./example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/posix/src/ctype/islower_test.d \
./example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/posix/src/ctype/isxdigit_test.d \
./example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/posix/src/ctype/tolower_test.d \
./example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/posix/src/ctype/toupper_test.d 

OBJS += \
./example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/posix/src/ctype/ctype_func_test.o \
./example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/posix/src/ctype/isdigit_test.o \
./example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/posix/src/ctype/islower_test.o \
./example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/posix/src/ctype/isxdigit_test.o \
./example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/posix/src/ctype/tolower_test.o \
./example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/posix/src/ctype/toupper_test.o 

DIR_OBJS += \
./example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/posix/src/ctype/*.o \

DIR_DEPS += \
./example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/posix/src/ctype/*.d \

DIR_EXPANDS += \
./example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/posix/src/ctype/*.234r.expand \


# Each subdirectory must supply rules for building sources it contributes
example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/posix/src/ctype/%.o: ../example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/posix/src/ctype/%.c
	@	riscv-none-embed-gcc -march=rv32imacxw -mabi=ilp32 -msmall-data-limit=8 -msave-restore -fmax-errors=20 -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -Wuninitialized -g -I"c:/GitProject/CH32V303CBT/Debug" -I"c:/GitProject/CH32V303CBT/Core" -I"c:/GitProject/CH32V303CBT/User" -I"c:/GitProject/CH32V303CBT/Peripheral/inc" -std=gnu99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"

