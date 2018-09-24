BIN    = air_drums
MCU    = atmega2560
F_CPU  = 16000000
BAUD   = 57600

PORT = /dev/ttyACM0
SPEED = 115200
PROGRAMMER = wiring

CC      = avr-g++
OBJCOPY = avr-objcopy
RM      = rm
MK      = mkdir

SRCDIR   = src
BUILDDIR = build
BINDIR   = bin
INCDIR   = $(SRCDIR)/include

TARGET = $(BINDIR)/$(BIN)

CPP_SOURCES  = $(wildcard $(SRCDIR)/*.cpp)
C_SOURCES = $(wildcard $(SRCDIR)/*.c)
CPP_OBJECTS  = $(addprefix $(BUILDDIR)/,$(notdir $(CPP_SOURCES:.cpp=.o)))
C_OBJECTS = $(addprefix $(BUILDDIR)/,$(notdir $(C_SOURCES:.c=.o)))
CFLAGS   = -g -Wall -mmcu=$(MCU) -Os
INCLUDES = -I$(INCDIR) -I$(LCDDIR)
LDFLAGS  = -g -mmcu=$(MCU) -Wl,-u,vfprintf -lprintf_flt -lm
DEFINES  = -DF_CPU=$(F_CPU) -DBAUD=$(BAUD) -DDRV_MMC=0

all: $(TARGET).hex size

size:
	@if [ -f $(TARGET).elf ]; then avr-size -C --mcu=$(MCU) $(TARGET).elf; fi

clean:
	@echo "Cleaning..."
	@$(RM) -rf $(BUILDDIR) $(BINDIR)

$(TARGET).hex: $(TARGET).elf
	@echo "Generating hex..."
	@$(RM) -f $(TARGET).hex
	@$(OBJCOPY) -j .text -j .data -O ihex $< $@

$(TARGET).elf: $(CPP_OBJECTS) $(C_OBJECTS)
	@echo "Linking..."
	@$(MK) -p $(BINDIR)
	@$(CC) $^ -o $@ $(LDFLAGS)

$(BUILDDIR)/%.o: $(SRCDIR)/%.c
	@echo "Compiling $<"
	@$(MK) -p $(BUILDDIR)
	@$(CC) -c $< -o $@ $(INCLUDES) $(CFLAGS) $(DEFINES)

$(BUILDDIR)/%.o: $(SRCDIR)/%.cpp
	@echo "Compiling $<"
	@$(MK) -p $(BUILDDIR)
	@$(CC) -c $< -o $@ $(INCLUDES) $(CFLAGS) $(DEFINES)


flash:
	@avrdude -c $(PROGRAMMER) -p $(MCU) -P $(PORT) -b $(SPEED) -U flash:w:$(TARGET).hex

monitor:
	@screen $(PORT) $(BAUD)