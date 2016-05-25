#
#             LUFA Library
#     Copyright (C) Dean Camera, 2013.
#
#  dean [at] fourwalledcubicle [dot] com
#           www.lufa-lib.org
#
# --------------------------------------
#         LUFA Project Makefile.
# --------------------------------------

# Run "make help" for target help.

MCU			= atmega32u2
ARCH		= AVR8
BOARD       = USER
F_CPU       = 8000000
F_USB       = $(F_CPU)
OPTIMIZATION = 3
TARGET      = Turtle
SRC         = Turtle.c Descriptors.c DeviceStandardReq.c CDCClassDevice.c
SRC			+= USBController_AVR8.c USBInterrupt_AVR8.c ConfigDescriptors.c Events.c
SRC			+= USBTask.c HIDParser.c Endpoint_AVR8.c EndpointStream_AVR8.c
SRC			+= flash.c serialio.c sd.c pff.c
LUFA_PATH    = ./
CC_FLAGS     = -DUSE_LUFA_CONFIG_HEADER -IConfig/
LD_FLAGS     =

# Default target
all:

include lufa_build.mk

