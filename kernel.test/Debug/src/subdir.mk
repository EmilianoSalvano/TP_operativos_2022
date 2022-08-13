################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
/home/utnso/git/tp-2022-1c-Los-o-os/kernel/src/iconsole.c \
/home/utnso/git/tp-2022-1c-Los-o-os/kernel/src/icpu.c \
/home/utnso/git/tp-2022-1c-Los-o-os/kernel/src/kernel.c \
../src/kernel.test.c \
/home/utnso/git/tp-2022-1c-Los-o-os/kernel/src/kernel_global.c \
/home/utnso/git/tp-2022-1c-Los-o-os/kernel/src/imemory.c \
../src/mock.console.c \
../src/mock.cpu.c \
../src/mock.memory.c \
/home/utnso/git/tp-2022-1c-Los-o-os/kernel/src/planner.c 

OBJS += \
./src/iconsole.o \
./src/icpu.o \
./src/kernel.o \
./src/kernel.test.o \
./src/kernel_global.o \
./src/imemory.o \
./src/mock.console.o \
./src/mock.cpu.o \
./src/mock.memory.o \
./src/planner.o 

C_DEPS += \
./src/iconsole.d \
./src/icpu.d \
./src/kernel.d \
./src/kernel.test.d \
./src/kernel_global.d \
./src/memory.d \
./src/mock.console.d \
./src/mock.cpu.d \
./src/mock.memory.d \
./src/planner.d 


# Each subdirectory must supply rules for building sources it contributes
src/iconsole.o: /home/utnso/git/tp-2022-1c-Los-o-os/kernel/src/iconsole.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I"/home/utnso/git/tp-2022-1c-Los-o-os/kernel/include" -I"/home/utnso/git/tp-2022-1c-Los-o-os/shared/include" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/icpu.o: /home/utnso/git/tp-2022-1c-Los-o-os/kernel/src/icpu.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I"/home/utnso/git/tp-2022-1c-Los-o-os/kernel/include" -I"/home/utnso/git/tp-2022-1c-Los-o-os/shared/include" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/kernel.o: /home/utnso/git/tp-2022-1c-Los-o-os/kernel/src/kernel.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I"/home/utnso/git/tp-2022-1c-Los-o-os/kernel/include" -I"/home/utnso/git/tp-2022-1c-Los-o-os/shared/include" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I"/home/utnso/git/tp-2022-1c-Los-o-os/kernel/include" -I"/home/utnso/git/tp-2022-1c-Los-o-os/shared/include" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/kernel_global.o: /home/utnso/git/tp-2022-1c-Los-o-os/kernel/src/kernel_global.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I"/home/utnso/git/tp-2022-1c-Los-o-os/kernel/include" -I"/home/utnso/git/tp-2022-1c-Los-o-os/shared/include" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/imemory.o: /home/utnso/git/tp-2022-1c-Los-o-os/kernel/src/imemory.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I"/home/utnso/git/tp-2022-1c-Los-o-os/kernel/include" -I"/home/utnso/git/tp-2022-1c-Los-o-os/shared/include" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/planner.o: /home/utnso/git/tp-2022-1c-Los-o-os/kernel/src/planner.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I"/home/utnso/git/tp-2022-1c-Los-o-os/kernel/include" -I"/home/utnso/git/tp-2022-1c-Los-o-os/shared/include" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


