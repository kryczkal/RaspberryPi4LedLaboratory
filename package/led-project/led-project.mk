# Version and Git Source
LED_PROJECT_VERSION = main
LED_PROJECT_SITE = https://github.com/kryczkal/RaspberryPi4LedLaboratory
LED_PROJECT_SITE_METHOD = git

# Standard Metadata
LED_PROJECT_LICENSE = MIT
LED_PROJECT_LICENSE_FILES = LICENSE # Path relative to the source root

# Dependencies
LED_PROJECT_DEPENDENCIES = c-periphery

# CMake Build Options
LED_PROJECT_CMAKE_OPTS = -DCMAKE_BUILD_TYPE=Debug \

# Use the CMake package infrastructure
$(eval $(cmake-package))