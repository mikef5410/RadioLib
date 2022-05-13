
WIRING_PI=/usr/local
VPATH=.:modules/CC1101:modules/LLCC68:modules/nRF24:modules/RF69
VPATH+=:modules/RFM2x:modules/RFM9x:modules/Si443x:modules/SX1231
VPATH+=:modules/SX126x:modules/SX127x:modules/SX128x:protocols/AFSK
VPATH+=:protocols/APRS:protocols/AX25:protocols/FSK4
VPATH+=:protocols/Hellschreiber:protocols/Morse
VPATH+=:protocols/PhysicalLayer:protocols/RTTY:protocols/SSTV

INCLUDES=-I/usr/local/include

LIBS=-L/usr/local/lib -lwiringPi

cscope id etags tags: FILETYPES:=-iname "*.cpp" -o -iname "*.hpp" -o -name "*.S" -o -iname "*.c" -o -iname "*.h" -o -name "*.s" -o -name "*.cc"
cscope id etags tags: INDEX_FILES=$(shell find -L $(subst :, ,$(VPATH)) $(FILETYPES))

CPPFLAGS+=$(INCLUDES)
CFLAGS+= -fPIC -Wall -g
CFLAGS+= -Wredundant-decls -Wswitch-default -fpermissive
CFLAGS+= -DUSE_WIRINGPI
CXXFLAGS=$(CFLAGS)

SRCS:=$(shell find . -name "*.cpp" -printf "%f ")
SRCS:=$(filter-out Module_%.cpp, $(SRCS))
SRCS:=$(filter-out Morse.cpp RTTY.cpp Hellschreiber.cpp, $(SRCS))
OBJS_DIR=build
#LIBOBJ:=$(addprefix $(OBJS_DIR)/, $(patsubst %.cpp,%.o,$(SRCS)))
LIBOBJ:=$(patsubst %.cpp,%.o,$(SRCS))

include mk/deps.mk

all: libRadioLib.so

$(OBJS_DIR):
	mkdir -p $@


libRadioLib.so: $(LIBOBJ)
	g++ -shared -o$@ $(LIBOBJ)


clean:
	find . -name "*~" -o -name "#*" -o -name ".#*" | xargs rm -f
	rm -f *.o

include mk/tags.mk

include $(wildcard $(patsubst %,$(DEPDIR)/%.d,$(basename $(SRCS))))	
