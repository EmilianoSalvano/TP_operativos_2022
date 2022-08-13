################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/cpu.c \
../src/main.c \
../src/memory.c \
../src/monitor.test.c \
../src/shared.test.c \
../src/simple.socket.test.c \
../src/socket.handler.test.c \
../src/socket.test.c \
../src/thread.test.c \
../src/timer.test.c 

OBJS += \
./src/cpu.o \
./src/main.o \
./src/memory.o \
./src/monitor.test.o \
./src/shared.test.o \
./src/simple.socket.test.o \
./src/socket.handler.test.o \
./src/socket.test.o \
./src/thread.test.o \
./src/timer.test.o 

C_DEPS += \
./src/cpu.d \
./src/main.d \
./src/memory.d \
./src/monitor.test.d \
./src/shared.test.d \
./src/simple.socket.test.d \
./src/socket.handler.test.d \
./src/socket.test.d \
./src/thread.test.d \
./src/timer.test.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I"/home/utnso/git/tp-2022-1c-Los-o-os/shared" -I"/home/utnso/git/tp-2022-1c-Los-o-os/shared/include" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


