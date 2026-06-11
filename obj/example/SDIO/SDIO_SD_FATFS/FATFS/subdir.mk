################################################################################
# MRS Version: 2.4.0
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../example/SDIO/SDIO_SD_FATFS/FATFS/diskio.c \
../example/SDIO/SDIO_SD_FATFS/FATFS/ff.c \
../example/SDIO/SDIO_SD_FATFS/FATFS/ffsystem.c \
../example/SDIO/SDIO_SD_FATFS/FATFS/ffunicode.c 

C_DEPS += \
./example/SDIO/SDIO_SD_FATFS/FATFS/diskio.d \
./example/SDIO/SDIO_SD_FATFS/FATFS/ff.d \
./example/SDIO/SDIO_SD_FATFS/FATFS/ffsystem.d \
./example/SDIO/SDIO_SD_FATFS/FATFS/ffunicode.d 

OBJS += \
./example/SDIO/SDIO_SD_FATFS/FATFS/diskio.o \
./example/SDIO/SDIO_SD_FATFS/FATFS/ff.o \
./example/SDIO/SDIO_SD_FATFS/FATFS/ffsystem.o \
./example/SDIO/SDIO_SD_FATFS/FATFS/ffunicode.o 

DIR_OBJS += \
./example/SDIO/SDIO_SD_FATFS/FATFS/*.o \

DIR_DEPS += \
./example/SDIO/SDIO_SD_FATFS/FATFS/*.d \

DIR_EXPANDS += \
./example/SDIO/SDIO_SD_FATFS/FATFS/*.234r.expand \


# Each subdirectory must supply rules for building sources it contributes
example/SDIO/SDIO_SD_FATFS/FATFS/%.o: ../example/SDIO/SDIO_SD_FATFS/FATFS/%.c
	@	riscv-none-embed-gcc -march=rv32imacxw -mabi=ilp32 -msmall-data-limit=8 -msave-restore -fmax-errors=20 -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -Wuninitialized -g -I"c:/GitProject/CH32V303CBT/Debug" -I"c:/GitProject/CH32V303CBT/Core" -I"c:/GitProject/CH32V303CBT/User" -I"c:/GitProject/CH32V303CBT/Peripheral/inc" -std=gnu99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"

