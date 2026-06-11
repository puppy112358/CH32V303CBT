################################################################################
# MRS Version: 2.4.0
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/fuzz/src/ctype/isdigit_fuzz.c \
../example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/fuzz/src/ctype/islower_fuzz.c \
../example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/fuzz/src/ctype/isxdigit_fuzz.c \
../example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/fuzz/src/ctype/test_isalnum_fuzz.c \
../example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/fuzz/src/ctype/test_isascii_fuzz.c \
../example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/fuzz/src/ctype/test_isprint_fuzz.c \
../example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/fuzz/src/ctype/test_isspace_fuzz.c \
../example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/fuzz/src/ctype/test_isupper_fuzz.c \
../example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/fuzz/src/ctype/tolower_fuzz.c \
../example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/fuzz/src/ctype/toupper_fuzz.c 

C_DEPS += \
./example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/fuzz/src/ctype/isdigit_fuzz.d \
./example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/fuzz/src/ctype/islower_fuzz.d \
./example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/fuzz/src/ctype/isxdigit_fuzz.d \
./example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/fuzz/src/ctype/test_isalnum_fuzz.d \
./example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/fuzz/src/ctype/test_isascii_fuzz.d \
./example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/fuzz/src/ctype/test_isprint_fuzz.d \
./example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/fuzz/src/ctype/test_isspace_fuzz.d \
./example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/fuzz/src/ctype/test_isupper_fuzz.d \
./example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/fuzz/src/ctype/tolower_fuzz.d \
./example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/fuzz/src/ctype/toupper_fuzz.d 

OBJS += \
./example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/fuzz/src/ctype/isdigit_fuzz.o \
./example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/fuzz/src/ctype/islower_fuzz.o \
./example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/fuzz/src/ctype/isxdigit_fuzz.o \
./example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/fuzz/src/ctype/test_isalnum_fuzz.o \
./example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/fuzz/src/ctype/test_isascii_fuzz.o \
./example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/fuzz/src/ctype/test_isprint_fuzz.o \
./example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/fuzz/src/ctype/test_isspace_fuzz.o \
./example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/fuzz/src/ctype/test_isupper_fuzz.o \
./example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/fuzz/src/ctype/tolower_fuzz.o \
./example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/fuzz/src/ctype/toupper_fuzz.o 

DIR_OBJS += \
./example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/fuzz/src/ctype/*.o \

DIR_DEPS += \
./example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/fuzz/src/ctype/*.d \

DIR_EXPANDS += \
./example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/fuzz/src/ctype/*.234r.expand \


# Each subdirectory must supply rules for building sources it contributes
example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/fuzz/src/ctype/%.o: ../example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/fuzz/src/ctype/%.c
	@	riscv-none-embed-gcc -march=rv32imacxw -mabi=ilp32 -msmall-data-limit=8 -msave-restore -fmax-errors=20 -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -Wuninitialized -g -I"c:/GitProject/CH32V303CBT/Debug" -I"c:/GitProject/CH32V303CBT/Core" -I"c:/GitProject/CH32V303CBT/User" -I"c:/GitProject/CH32V303CBT/Peripheral/inc" -std=gnu99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"

