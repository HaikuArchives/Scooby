# Application Name
NAME = Scooby
# Resources
RSRCS= ./resources/Icon.rsrc ./resources/Resource.rsrc
# Libraries
LIBS= root be netapi net tracker mail
# Library path
LIBPATHS= 
# System include path
SYSTEM_INCLUDE_PATHS =
# Local include path 
LOCAL_INCLUDE_PATHS = ./libs/ComboBox ./libs/Utils ./libs/Santa ./libs/Toolbar
# OPTIMIZE
OPTIMIZE= FULL
# Defines
DEFINES= DEBUG USE_SCANDIR
# USE_SPLOCALE: Enable SpLocale supprt
# USE_ICONV:	Enable iconv support
USE_SPLOCALE = 0
USE_ICONV = 0
# Warnings
WARNINGS = ALL
# Sources
-include .all_sources
include .makefile.base
