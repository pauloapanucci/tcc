CC            = g++
GCC           = gcc
CFLAGS        = -DRASPBERRY_PI -DBCM2835_NO_DELAY_COMPATIBILITY -D__BASEFILE__=\"$*\"
LIBS          = -lbcm2835
DEBUG = @

APP_DIR = .
APP_INCLUDE_DIRS += -I $(APP_DIR)
APP_NAME = main
APP_SRC_FILES = $(APP_NAME).cpp

# AWS client directory
AWS = ./aws

PLATFORM_DIR = $(AWS)/platform/linux/mbedtls
PLATFORM_COMMON_DIR = $(AWS)/platform/linux/common

IOT_INCLUDE_DIRS += -I $(AWS)/include
IOT_INCLUDE_DIRS += -I $(AWS)/certs
IOT_INCLUDE_DIRS += -I $(AWS)/external_libs/jsmn
IOT_INCLUDE_DIRS += -I $(PLATFORM_COMMON_DIR)
IOT_INCLUDE_DIRS += -I $(PLATFORM_DIR)

IOT_SRC_FILES += $(shell find $(AWS)/src/ -name '*.c')
IOT_SRC_FILES += $(shell find $(AWS)/external_libs/jsmn -name '*.c')
IOT_SRC_FILES += $(shell find $(PLATFORM_DIR)/ -name '*.c')
IOT_SRC_FILES += $(shell find $(PLATFORM_COMMON_DIR)/ -name '*.c')

#TLS - mbedtls
MBEDTLS_DIR = $(AWS)/external_libs/mbedTLS
TLS_LIB_DIR = $(MBEDTLS_DIR)/library
TLS_INCLUDE_DIR = -I $(MBEDTLS_DIR)/include
EXTERNAL_LIBS += -L$(TLS_LIB_DIR)
LD_FLAG += -Wl,-rpath,$(TLS_LIB_DIR)
LD_FLAG += -ldl $(TLS_LIB_DIR)/libmbedtls.a $(TLS_LIB_DIR)/libmbedcrypto.a $(TLS_LIB_DIR)/libmbedx509.a

# RADIOHEADBASE = ./RadioHead
# INCLUDE = -I$(RADIOHEADBASE)

#Aggregate all include and src directories
INCLUDE_ALL_DIRS += $(IOT_INCLUDE_DIRS)
INCLUDE_ALL_DIRS += $(TLS_INCLUDE_DIR)
INCLUDE_ALL_DIRS += $(APP_INCLUDE_DIRS)
# INCLUDE_ALL_DIRS += -I$(RADIOHEADBASE) 

# SRC_FILES += $(APP_SRC_FILES)
SRC_FILES += $(IOT_SRC_FILES)

# Logging level control
LOG_FLAGS += -DENABLE_IOT_DEBUG
LOG_FLAGS += -DENABLE_IOT_INFO
LOG_FLAGS += -DENABLE_IOT_WARN
LOG_FLAGS += -DENABLE_IOT_ERROR

COMPILER_FLAGS += -g 
COMPILER_FLAGS += $(LOG_FLAGS)
#If the processor is big endian uncomment the compiler flag
#COMPILER_FLAGS += -DREVERSED

MBED_TLS_MAKE_CMD = $(MAKE) -C $(MBEDTLS_DIR)

RADIOHEADBASE = ./RadioHead
INCLUDE = -I$(RADIOHEADBASE) 

PRE_MAKE_CMD = $(MBED_TLS_MAKE_CMD)
# MAKE_CMD = $(CC) $(SRC_FILES) $(COMPILER_FLAGS) -o main $(LD_FLAG) $(EXTERNAL_LIBS) $(INCLUDE_ALL_DIRS)
MAKE_CMD = @$(CC) $(SRC_FILES) $(COMPILER_FLAGS) -o main $(LD_FLAG) $(EXTERNAL_LIBS) $(INCLUDE_ALL_DIRS)

# all: rf95_client main
# all: $(PRE_MAKE_CMD) 
# 		main 
# 		$(POST_MAKE_CMD)
# all: $(PRE_MAKE_CMD) main
all:
	$(PRE_MAKE_CMD)
		# main
		# $(POST_MAKE_CMD)
pre: 
	$(PRE_MAKE_CMD)

aws.o: $(SRC_FILES)
			$(CC) $^ $(CFLAGS) $(COMPILER_FLAGS) -c $(EXTERNAL_LIBS) $(INCLUDE_ALL_DIRS) $<

RasPi.o: $(RADIOHEADBASE)/RHutil/RasPi.cpp
				$(CC) $(CFLAGS) -c $(RADIOHEADBASE)/RHutil/RasPi.cpp $(INCLUDE)

# aws.o: $(AWS)/src/*.c $(AWS)/external_libs/jsmn/*.c $(PLATFORM_DIR)/*.c $(PLATFORM_COMMON_DIR)/*.c
# 			$(CC)  $(COMPILER_FLAGS) -c $(EXTERNAL_LIBS) $(INCLUDE_ALL_DIRS) $<	

main.o: main.cpp
			$(CC) $(CFLAGS) -c $(INCLUDE) $(EXTERNAL_LIBS) $(INCLUDE_ALL_DIRS) $<

RH_RF95.o: $(RADIOHEADBASE)/RH_RF95.cpp
				$(CC) $(CFLAGS) -c $(INCLUDE) $<

RHDatagram.o: $(RADIOHEADBASE)/RHDatagram.cpp
				$(CC) $(CFLAGS) -c $(INCLUDE) $<

RHHardwareSPI.o: $(RADIOHEADBASE)/RHHardwareSPI.cpp
				$(CC) $(CFLAGS) -c $(INCLUDE) $<

RHSPIDriver.o: $(RADIOHEADBASE)/RHSPIDriver.cpp
				$(CC) $(CFLAGS) -c $(INCLUDE) $<

RHGenericDriver.o: $(RADIOHEADBASE)/RHGenericDriver.cpp
				$(CC) $(CFLAGS) -c $(INCLUDE) $<

RHGenericSPI.o: $(RADIOHEADBASE)/RHGenericSPI.cpp
				$(CC) $(CFLAGS) -c $(INCLUDE) $<

rf95_client: rf95_client.o RH_RF95.o RasPi.o RHHardwareSPI.o RHGenericDriver.o RHGenericSPI.o RHSPIDriver.o
				$(CC) $^ $(LIBS) -o rf95_client
				$(CC) $^ $(LIBS) -o main

main: *.o main.o RH_RF95.o RasPi.o RHHardwareSPI.o RHGenericDriver.o RHGenericSPI.o RHSPIDriver.o
				$(CC) $^ $(EXTERNAL_LIBS) $(INCLUDE_ALL_DIRS) -I. $(LIBS) -o main $(LD_FLAG) $(COMPILER_FLAGS)

clean:
				rm -rf *.o rf95_client main

# main: main.o RH_RF95.o RasPi.o RHHardwareSPI.o RHGenericDriver.o RHGenericSPI.o RHSPIDriver.o
# 

# main.o: main.cpp
# 				$(CC) $(CFLAGS) -c $(INCLUDE) $<