
INSTALL_PREFIX=/usr/local
WIRING_PI=/usr/local
DEVICE_ACCESS:=../../linux_device_access
VPATH=.:$(DEVICE_ACCESS):modules/CC1101:modules/LLCC68:modules/nRF24:modules/RF69
VPATH+=:modules/RFM2x:modules/RFM9x:modules/Si443x:modules/SX1231
VPATH+=:modules/SX126x:modules/SX127x:modules/SX128x:protocols/AFSK
VPATH+=:protocols/APRS:protocols/AX25:protocols/FSK4
VPATH+=:protocols/Hellschreiber:protocols/Morse
VPATH+=:protocols/PhysicalLayer:protocols/RTTY:protocols/SSTV

#INCLUDES=-I/usr/local/include
#LIBS=-L/usr/local/lib -lwiringPi
INCLUDES=
LIBS=

cscope id etags tags: FILETYPES:=-iname "*.cpp" -o -iname "*.hpp" -o -name "*.S" -o -iname "*.c" -o -iname "*.h" -o -name "*.s" -o -name "*.cc"
cscope id etags tags: INDEX_FILES=$(shell find -L $(subst :, ,$(VPATH)) $(FILETYPES))

CPPFLAGS += $(INCLUDES)
CPPFLAGS += $(patsubst %,-I%,$(subst :, ,$(VPATH)))
CFLAGS += -fPIC -Wall -g
CFLAGS += -Wredundant-decls -Wswitch-default
CFLAGS += -DUSE_WIRINGPI $(LIBS)
CXXFLAGS = $(CFLAGS) -fpermissive

SRCS:=$(shell find . -name "*.cpp" -printf "%f ")
SRCS:=$(filter-out Module_%.cpp, $(SRCS))
SRCS:=$(filter-out Morse.cpp RTTY.cpp Hellschreiber.cpp, $(SRCS))
#SRCS:=spi.c $(SRCS)

OBJS_DIR:=build
$(shell mkdir -p $(OBJS_DIR))
LIBOBJ:=$(patsubst %.cpp,%.o,$(SRCS))
LIBOBJ:=$(patsubst %.c,%.o,$(LIBOBJ))
LIBOBJ:=$(addprefix $(OBJS_DIR)/, $(LIBOBJ))

include mk/deps.mk

all: libRadioLib.so

$(OBJS_DIR):
	mkdir -p $@


libRadioLib.so: $(LIBOBJ)
	g++ -shared -o$@ $(LIBOBJ) 

install: libRadioLib.so
	cp libRadioLib.so $(INSTALL_PREFIX)/lib
	rm -rf  $(INSTALL_PREFIX)/include/RadioLib
	mkdir -p $(INSTALL_PREFIX)/include/RadioLib
	#find . -name "*.h" -exec cp {} $(INSTALL_PREFIX)/include/RadioLib \;
	find . -name "*.h" | cpio -pdumv $(INSTALL_PREFIX)/include/RadioLib

clean:
	find . -name "*~" -o -name "#*" -o -name ".#*" | xargs rm -f
	rm -f *.o *.so
	rm -rf $(OBJS_DIR)

include mk/rules.mk
include mk/tags.mk

include $(wildcard $(patsubst %,$(DEPDIR)/%.d,$(basename $(SRCS))))	
