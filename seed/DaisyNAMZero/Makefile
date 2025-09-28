# Project Name
TARGET = DaisyNAMZero

# Config
# APP_TYPE = BOOT_SRAM
DEBUG = 1

# Sources
CPP_SOURCES = DaisyNAMZero.cpp
CPP_SOURCES += dsp/ImpulseResponse.cpp
CPP_SOURCES += dsp/dspx.cpp
CPP_SOURCES += dsp/wav.cpp
#CPP_SOURCES += NAM/dsp.cpp
#CPP_SOURCES += NAM/get_dsp.cpp
#CPP_SOURCES += NAM/activations.cpp
#CPP_SOURCES += NAM/convnet.cpp
#CPP_SOURCES += NAM/lstm.cpp
#CPP_SOURCES += NAM/wavenet.cpp

# Library Locations
LIBDAISY_DIR = ../../libDaisy/
DAISYSP_DIR = ../../DaisySP/

# Core location, and generic Makefile.
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
include $(SYSTEM_FILES_DIR)/Makefile

CPPFLAGS += -I./AudioDSPTools/Dependencies/eigen 
CPPFLAGS += -I./AudioDSPTools/Dependencies/nlohmann 
CPPFLAGS += -fexceptions

# LDSCRIPT = ./STM32H750IB_custom_sram.lds