################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include src/subdir.mk
-include subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 

# All Target
all: kernel.test

dependents:
	-cd /home/utnso/git/tp-2022-1c-Los-o-os/shared/Debug && $(MAKE) all

# Tool invocations
kernel.test: $(OBJS) $(USER_OBJS) /home/utnso/git/tp-2022-1c-Los-o-os/shared/Debug/libshared.so
	@echo 'Building target: $@'
	@echo 'Invoking: GCC C Linker'
	gcc -L"/home/utnso/git/tp-2022-1c-Los-o-os/shared/Debug" -o "kernel.test" $(OBJS) $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) $(EXECUTABLES)$(OBJS)$(C_DEPS) kernel.test
	-@echo ' '

.PHONY: all clean dependents
/home/utnso/git/tp-2022-1c-Los-o-os/shared/Debug/libshared.so:

-include ../makefile.targets
