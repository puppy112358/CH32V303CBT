################################################################################
# MRS Version: 2.4.0
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/fuzz/src/string/memcmp_fuzz.c \
../example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/fuzz/src/string/memcpy_fuzz.c \
../example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/fuzz/src/string/memset_fuzz.c \
../example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/fuzz/src/string/strchr_fuzz.c \
../example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/fuzz/src/string/strcmp_fuzz.c \
../example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/fuzz/src/string/strcspn_fuzz.c \
../example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/fuzz/src/string/strdup_fuzz.c \
../example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/fuzz/src/string/strerror_fuzz.c \
../example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/fuzz/src/string/strstr_fuzz.c \
../example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/fuzz/src/string/test_strlen_fuzz.c \
../example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/fuzz/src/string/test_strncmp_fuzz.c \
../example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/fuzz/src/string/test_strrchr_fuzz.c 

C_DEPS += \
./example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/fuzz/src/string/memcmp_fuzz.d \
./example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/fuzz/src/string/memcpy_fuzz.d \
./example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/fuzz/src/string/memset_fuzz.d \
./example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/fuzz/src/string/strchr_fuzz.d \
./example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/fuzz/src/string/strcmp_fuzz.d \
./example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/fuzz/src/string/strcspn_fuzz.d \
./example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/fuzz/src/string/strdup_fuzz.d \
./example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/fuzz/src/string/strerror_fuzz.d \
./example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/fuzz/src/string/strstr_fuzz.d \
./example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/fuzz/src/string/test_strlen_fuzz.d \
./example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/fuzz/src/string/test_strncmp_fuzz.d \
./example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/fuzz/src/string/test_strrchr_fuzz.d 

OBJS += \
./example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/fuzz/src/string/memcmp_fuzz.o \
./example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/fuzz/src/string/memcpy_fuzz.o \
./example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/fuzz/src/string/memset_fuzz.o \
./example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/fuzz/src/string/strchr_fuzz.o \
./example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/fuzz/src/string/strcmp_fuzz.o \
./example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/fuzz/src/string/strcspn_fuzz.o \
./example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/fuzz/src/string/strdup_fuzz.o \
./example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/fuzz/src/string/strerror_fuzz.o \
./example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/fuzz/src/string/strstr_fuzz.o \
./example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/fuzz/src/string/test_strlen_fuzz.o \
./example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/fuzz/src/string/test_strncmp_fuzz.o \
./example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/fuzz/src/string/test_strrchr_fuzz.o 

DIR_OBJS += \
./example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/fuzz/src/string/*.o \

DIR_DEPS += \
./example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/fuzz/src/string/*.d \

DIR_EXPANDS += \
./example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/fuzz/src/string/*.234r.expand \


# Each subdirectory must supply rules for building sources it contributes
example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/fuzz/src/string/%.o: ../example/HarmonyOS/LiteOS_m/LiteOS/testsuits/unittest/fuzz/src/string/%.c
	@	riscv-none-embed-gcc -march=rv32imacxw -mabi=ilp32 -msmall-data-limit=8 -msave-restore -fmax-errors=20 -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -Wuninitialized -g -I"c:/GitProject/CH32V303CBT/Debug" -I"c:/GitProject/CH32V303CBT/Core" -I"c:/GitProject/CH32V303CBT/User" -I"c:/GitProject/CH32V303CBT/Peripheral/inc" -std=gnu99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"

