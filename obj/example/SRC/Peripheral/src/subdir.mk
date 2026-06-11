################################################################################
# MRS Version: 2.4.0
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../example/SRC/Peripheral/src/ch32v30x_adc.c \
../example/SRC/Peripheral/src/ch32v30x_bkp.c \
../example/SRC/Peripheral/src/ch32v30x_can.c \
../example/SRC/Peripheral/src/ch32v30x_crc.c \
../example/SRC/Peripheral/src/ch32v30x_dac.c \
../example/SRC/Peripheral/src/ch32v30x_dbgmcu.c \
../example/SRC/Peripheral/src/ch32v30x_dma.c \
../example/SRC/Peripheral/src/ch32v30x_dvp.c \
../example/SRC/Peripheral/src/ch32v30x_eth.c \
../example/SRC/Peripheral/src/ch32v30x_exti.c \
../example/SRC/Peripheral/src/ch32v30x_flash.c \
../example/SRC/Peripheral/src/ch32v30x_fsmc.c \
../example/SRC/Peripheral/src/ch32v30x_gpio.c \
../example/SRC/Peripheral/src/ch32v30x_i2c.c \
../example/SRC/Peripheral/src/ch32v30x_iwdg.c \
../example/SRC/Peripheral/src/ch32v30x_misc.c \
../example/SRC/Peripheral/src/ch32v30x_opa.c \
../example/SRC/Peripheral/src/ch32v30x_pwr.c \
../example/SRC/Peripheral/src/ch32v30x_rcc.c \
../example/SRC/Peripheral/src/ch32v30x_rng.c \
../example/SRC/Peripheral/src/ch32v30x_rtc.c \
../example/SRC/Peripheral/src/ch32v30x_sdio.c \
../example/SRC/Peripheral/src/ch32v30x_spi.c \
../example/SRC/Peripheral/src/ch32v30x_tim.c \
../example/SRC/Peripheral/src/ch32v30x_usart.c \
../example/SRC/Peripheral/src/ch32v30x_wwdg.c 

C_DEPS += \
./example/SRC/Peripheral/src/ch32v30x_adc.d \
./example/SRC/Peripheral/src/ch32v30x_bkp.d \
./example/SRC/Peripheral/src/ch32v30x_can.d \
./example/SRC/Peripheral/src/ch32v30x_crc.d \
./example/SRC/Peripheral/src/ch32v30x_dac.d \
./example/SRC/Peripheral/src/ch32v30x_dbgmcu.d \
./example/SRC/Peripheral/src/ch32v30x_dma.d \
./example/SRC/Peripheral/src/ch32v30x_dvp.d \
./example/SRC/Peripheral/src/ch32v30x_eth.d \
./example/SRC/Peripheral/src/ch32v30x_exti.d \
./example/SRC/Peripheral/src/ch32v30x_flash.d \
./example/SRC/Peripheral/src/ch32v30x_fsmc.d \
./example/SRC/Peripheral/src/ch32v30x_gpio.d \
./example/SRC/Peripheral/src/ch32v30x_i2c.d \
./example/SRC/Peripheral/src/ch32v30x_iwdg.d \
./example/SRC/Peripheral/src/ch32v30x_misc.d \
./example/SRC/Peripheral/src/ch32v30x_opa.d \
./example/SRC/Peripheral/src/ch32v30x_pwr.d \
./example/SRC/Peripheral/src/ch32v30x_rcc.d \
./example/SRC/Peripheral/src/ch32v30x_rng.d \
./example/SRC/Peripheral/src/ch32v30x_rtc.d \
./example/SRC/Peripheral/src/ch32v30x_sdio.d \
./example/SRC/Peripheral/src/ch32v30x_spi.d \
./example/SRC/Peripheral/src/ch32v30x_tim.d \
./example/SRC/Peripheral/src/ch32v30x_usart.d \
./example/SRC/Peripheral/src/ch32v30x_wwdg.d 

OBJS += \
./example/SRC/Peripheral/src/ch32v30x_adc.o \
./example/SRC/Peripheral/src/ch32v30x_bkp.o \
./example/SRC/Peripheral/src/ch32v30x_can.o \
./example/SRC/Peripheral/src/ch32v30x_crc.o \
./example/SRC/Peripheral/src/ch32v30x_dac.o \
./example/SRC/Peripheral/src/ch32v30x_dbgmcu.o \
./example/SRC/Peripheral/src/ch32v30x_dma.o \
./example/SRC/Peripheral/src/ch32v30x_dvp.o \
./example/SRC/Peripheral/src/ch32v30x_eth.o \
./example/SRC/Peripheral/src/ch32v30x_exti.o \
./example/SRC/Peripheral/src/ch32v30x_flash.o \
./example/SRC/Peripheral/src/ch32v30x_fsmc.o \
./example/SRC/Peripheral/src/ch32v30x_gpio.o \
./example/SRC/Peripheral/src/ch32v30x_i2c.o \
./example/SRC/Peripheral/src/ch32v30x_iwdg.o \
./example/SRC/Peripheral/src/ch32v30x_misc.o \
./example/SRC/Peripheral/src/ch32v30x_opa.o \
./example/SRC/Peripheral/src/ch32v30x_pwr.o \
./example/SRC/Peripheral/src/ch32v30x_rcc.o \
./example/SRC/Peripheral/src/ch32v30x_rng.o \
./example/SRC/Peripheral/src/ch32v30x_rtc.o \
./example/SRC/Peripheral/src/ch32v30x_sdio.o \
./example/SRC/Peripheral/src/ch32v30x_spi.o \
./example/SRC/Peripheral/src/ch32v30x_tim.o \
./example/SRC/Peripheral/src/ch32v30x_usart.o \
./example/SRC/Peripheral/src/ch32v30x_wwdg.o 

DIR_OBJS += \
./example/SRC/Peripheral/src/*.o \

DIR_DEPS += \
./example/SRC/Peripheral/src/*.d \

DIR_EXPANDS += \
./example/SRC/Peripheral/src/*.234r.expand \


# Each subdirectory must supply rules for building sources it contributes
example/SRC/Peripheral/src/%.o: ../example/SRC/Peripheral/src/%.c
	@	riscv-none-embed-gcc -march=rv32imacxw -mabi=ilp32 -msmall-data-limit=8 -msave-restore -fmax-errors=20 -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -Wuninitialized -g -I"c:/GitProject/CH32V303CBT/Debug" -I"c:/GitProject/CH32V303CBT/Core" -I"c:/GitProject/CH32V303CBT/User" -I"c:/GitProject/CH32V303CBT/Peripheral/inc" -std=gnu99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"

