# Application Name
NAME = Scooby

# Resources
RSRCS= ./resources/Icon.rsrc ./resources/Resource.rsrc

# Libraries
REG_LIBS = root be tracker
NET_LIBS = netapi net
BONE_LIBS = bnetapi bind socket
# if libbind.so exists, assume we are running BONE
ifeq ($(wildcard /system/lib/libbind.so),)
LIBS= $(REG_LIBS) $(NET_LIBS)
else
LIBS= $(REG_LIBS) $(BONE_LIBS)
endif

# Library path
LIBPATHS= 

# System include path
SYSTEM_INCLUDE_PATHS =

# Local include path 
LOCAL_INCLUDE_PATHS = ./libs/ComboBox ./libs/Utils ./libs/Santa ./libs/Toolbar

# OPTIMIZE
OPTIMIZE= FULL

# Defines
# if libbind.so exists, assume we are running BONE
ifeq ($(wildcard /system/lib/libbind.so),)
DEFINES= DEBUG=1 USE_SCANDIR=1
else
DEFINES= DEBUG=1 USE_SCANDIR=1 B_BEOS_BONE=1
endif

# Linker flag required by BONE
ifneq ($(wildcard /system/lib/libbind.so),)
LDFLAGS= -nodefaultlibs
endif

# USE_SPLOCALE: Enable SpLocale supprt
# USE_ICONV:	Enable iconv support
USE_SPLOCALE = 0
USE_ICONV = 0

# Warnings
WARNINGS = ALL

# Sources
-include .all_sources
include .makefile.base
