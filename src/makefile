# Variables
CC ?= musl-gcc
CFLAGS ?= -Wall -I./iflib -m32
LDFLAGS ?= -static
SRC_DIR = .
LIB = $(SRC_DIR)/iflib/lib_interfaces.c
# LIB = ./iflib/lib_interfaces.c
OUTPUT_DIR = ../bin

# Applications et leurs répertoires sources
APPS = ifnetshow ifshow neighborshow

# Target par défaut
.PHONY: all clean build $(APPS)

# Build pour toutes les applications
all:
	@mkdir -p $(OUTPUT_DIR)
	$(MAKE) build

# Construire toutes les applications si 'build' est appelé sans arguments
build: $(APPS)

# Cibles spécifiques pour les applications
.PHONY: ifnetshow ifshow neighborshow

ifnetshow: 
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(OUTPUT_DIR)/ifnetshow-client $(SRC_DIR)/ifnetshow/client/main.c $(LIB)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(OUTPUT_DIR)/ifnetshow-server $(SRC_DIR)/ifnetshow/server/main.c $(LIB)

ifshow:
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(OUTPUT_DIR)/ifshow $(SRC_DIR)/ifshow/main.c $(LIB)

neighborshow:
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(OUTPUT_DIR)/nbsc $(SRC_DIR)/neighborshow/nbsc.c $(LIB)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(OUTPUT_DIR)/nbsd $(SRC_DIR)/neighborshow/nbsd.c $(LIB)

# Clean
clean:
	rm -rf $(OUTPUT_DIR)
