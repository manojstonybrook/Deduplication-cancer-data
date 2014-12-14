################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../cac/content_aware_chunking.c \
../cac/convert_file.c 

OBJS += \
./cac/content_aware_chunking.o \
./cac/convert_file.o 

C_DEPS += \
./cac/content_aware_chunking.d \
./cac/convert_file.d 


# Each subdirectory must supply rules for building sources it contributes
cac/%.o: ../cac/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I/home/manoj/Desktop/Project/files_system/fs/inc -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


