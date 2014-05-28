include Balau/common.mk

ifeq ($(DEBUG),)
CPPFLAGS += -g3 -gdwarf-2 -O3 -DNDEBUG
LDFLAGS += -g3 -gdwarf-2
else
CPPFLAGS += -g3 -gdwarf-2 -DDEBUG
LDFLAGS += -g3 -gdwarf-2
endif

INCLUDES = . src src/cpu/x86 src/Loaders Balau/includes Balau/libcoro Balau/libeio Balau/libev Balau/LuaJIT/src Balau/src/jsoncpp/include
LIBS = z uuid protobuf

ifeq ($(SYSTEM),Darwin)
    LIBS += pthread iconv
    CONFIG_H = Balau/darwin-config.h
endif

ifeq ($(SYSTEM),Linux)
    LIBS += pthread dl
    CONFIG_H = Balau/linux-config.h
endif

CPPFLAGS_NO_ARCH += $(addprefix -I, $(INCLUDES)) -fexceptions -imacros $(CONFIG_H)
CPPFLAGS += $(CPPFLAGS_NO_ARCH) $(ARCH_FLAGS)

LDFLAGS += $(ARCH_FLAGS)
LDLIBS = $(addprefix -l, $(LIBS))

vpath %.cpp src:src/cpu:src/cpu/x86:src/cpu/x86_capstone:src/Loaders/PE:src/Loaders/Elf:src/PDB
vpath %.cc src/protobufs
vpath %.proto src/protobufs

IGOR_SOURCES = \
Igor.cpp \
IgorSession.cpp \
IgorLocalSession.cpp \
IgorDatabase.cpp \
IgorSection.cpp \
IgorAnalysis.cpp \
IgorHttp.cpp \
IgorWS.cpp \
\
IgorProtoFile.pb.cc \
\
Loaders/PE/PELoader.cpp \
Loaders/Elf/elfLoader.cpp \
\
cpu/cpuModule.cpp \
cpu/x86/cpu_x86.cpp \
cpu/x86/cpu_x86_opcodes.cpp \
cpu/x86/cpu_x86_opcodes_F.cpp \
cpu/x86/cpu_x86_capstone.cpp \
\
PDB/gsi.cpp \
PDB/msf.cpp \
PDB/pdb.cpp \
PDB/sym.cpp \
PDB/tpi.cpp \


ALL_OBJECTS = $(addsuffix .o, $(notdir $(basename $(IGOR_SOURCES))))
ALL_DEPS = $(addsuffix .dep, $(notdir $(basename $(IGOR_SOURCES))))

TARGET=Igor.$(BINEXT)

all: dep Balau $(TARGET)

strip: $(TARGET)
	$(STRIP) $(TARGET)

Balau:
	$(MAKE) -C Balau

tests: all
	$(MAKE) -C Balau tests

Balau/libBalau.a:
	$(MAKE) -C Balau

capstone/libcapstone.a:
	capstone/make.sh

$(TARGET): Balau/libBalau.a capstone/libcapstone.a $(ALL_OBJECTS)
	$(LD) $(LDFLAGS) -o $@ $(ALL_OBJECTS) ./Balau/libBalau.a ./capstone/libcapstone.a ./Balau/LuaJIT/src/libluajit.a ./Balau/libtomcrypt/libtomcrypt.a ./Balau/libtommath/libtommath.a $(LDLIBS)

dep: $(ALL_DEPS)

%.dep: %.cpp
	$(CXX) $(CXXFLAGS) $(CPPFLAGS_NO_ARCH) -M $< > $@

%.dep: %.cc
	$(CXX) $(CXXFLAGS) $(CPPFLAGS_NO_ARCH) -M $< > $@

%.dep: %.c
	$(CC) $(CFLAGS) $(CPPFLAGS_NO_ARCH) -M $< > $@

%.pb.cc %.pb.h: %.proto
	protoc --cpp_out=. $<

-include $(ALL_DEPS)

clean:
	rm -f $(ALL_OBJECTS) $(ALL_DEPS) $(TARGET)
	$(MAKE) -C Balau clean

deepclean:
	git clean -f -d -x
	git submodule foreach git clean -f -d -x
	git submodule foreach git submodule foreach git clean -f -d -x
	git reset --hard HEAD
	git submodule foreach git reset --hard HEAD
	git submodule foreach git submodule foreach git reset --hard HEAD

.PHONY: clean deepclean strip Balau tests all
