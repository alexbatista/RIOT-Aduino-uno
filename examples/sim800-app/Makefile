# name of your application
APPLICATION = sim800-app

# If no BOARD is found in the environment, use this default:
BOARD ?= native

# This has to be the absolute path to the RIOT base directory:
RIOTBASE ?= $(CURDIR)/../..

BOARD ?=native

USEMODULE += xtimer
USEMODULE += fmt

FEATURES_REQUIRED += periph_timer
# Change this to 0 show compiler invocation lines by default:
QUIET ?= 1

include $(RIOTBASE)/Makefile.include
