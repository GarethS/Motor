
# Need to run bison first to generate valve.tab.h file which is included in output of flex (lex.yy.c).
# Build in a GNU bash shell v4.1.10(4)-release (i686-pc-cygwin) from Cygwin running under Windows 7

# 1 = debug build, 0 = release
myDebug := 1

.PHONY: all clean

prefix = ../../../../DriverLib/boards/ek-lm3s3748/motor
includeRTOS = ../../../../dev/docs/rtos/freertos/FreeRTOS/Demo/Common/drivers/LuminaryMicro/
includeGPIO = ../../../../DriverLib/src

ifeq ($(myDebug), 1)
GCC_FLAGS = -Wall
DEBUG_FLAGS = -D CYGWIN=1 -D DUMP=1
else
GCC_FLAGS =
DEBUG_FLAGS = -D CYGWIN=1
endif 


all: motor.exe

clean:
	rm motor.exe

motor.exe: $(prefix)/stepper.cpp $(prefix)/stepper.h $(prefix)/accel.cpp $(prefix)/accel.h motor.cpp log.cpp log.h lmi.cpp Makefile
	g++ $(GCC_FLAGS) $(DEBUG_FLAGS) -I. -I$(includeRTOS) -I$(includeGPIO) $(prefix)/stepper.cpp $(prefix)/accel.cpp motor.cpp log.cpp lmi.cpp -o motor.exe
