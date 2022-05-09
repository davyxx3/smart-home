################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (9-2020-q2-update)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Src/adxl362.c \
../Src/main.c \
../Src/modbus_master.c \
../Src/stm32l0xx_hal_msp.c \
../Src/stm32l0xx_hal_timebase_tim.c \
../Src/stm32l0xx_it.c \
../Src/syscalls.c \
../Src/sysmem.c \
../Src/system_stm32l0xx.c 

OBJS += \
./Src/adxl362.o \
./Src/main.o \
./Src/modbus_master.o \
./Src/stm32l0xx_hal_msp.o \
./Src/stm32l0xx_hal_timebase_tim.o \
./Src/stm32l0xx_it.o \
./Src/syscalls.o \
./Src/sysmem.o \
./Src/system_stm32l0xx.o 

C_DEPS += \
./Src/adxl362.d \
./Src/main.d \
./Src/modbus_master.d \
./Src/stm32l0xx_hal_msp.d \
./Src/stm32l0xx_hal_timebase_tim.d \
./Src/stm32l0xx_it.d \
./Src/syscalls.d \
./Src/sysmem.d \
./Src/system_stm32l0xx.d 


# Each subdirectory must supply rules for building sources it contributes
Src/%.o: ../Src/%.c Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m0plus -std=gnu11 -g3 -DUSE_HAL_DRIVER -DDEBUG -DSTM32L071xx -c -I../Inc -I../Drivers/CMSIS/Include -I../Drivers/STM32L0xx_HAL_Driver/Inc -I../Drivers/CMSIS/Device/ST/STM32L0xx/Include -I../Drivers/STM32L0xx_HAL_Driver/Inc/Legacy -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

clean: clean-Src

clean-Src:
	-$(RM) ./Src/adxl362.d ./Src/adxl362.o ./Src/main.d ./Src/main.o ./Src/modbus_master.d ./Src/modbus_master.o ./Src/stm32l0xx_hal_msp.d ./Src/stm32l0xx_hal_msp.o ./Src/stm32l0xx_hal_timebase_tim.d ./Src/stm32l0xx_hal_timebase_tim.o ./Src/stm32l0xx_it.d ./Src/stm32l0xx_it.o ./Src/syscalls.d ./Src/syscalls.o ./Src/sysmem.d ./Src/sysmem.o ./Src/system_stm32l0xx.d ./Src/system_stm32l0xx.o

.PHONY: clean-Src

