include Balau/common.mk

ifeq ($(DEBUG),)
CPPFLAGS += -g3 -gdwarf-2 -O3 -DNDEBUG
LDFLAGS += -g3 -gdwarf-2
else
CPPFLAGS += -g3 -gdwarf-2 -D_DEBUG
LDFLAGS += -g3 -gdwarf-2
endif

INCLUDES = \
. \
src src/cpu/x86 src/cpu/x86_llvm src/Loaders \
Balau/includes Balau/libcoro Balau/libeio Balau/libev Balau/LuaJIT/src Balau/src/jsoncpp/include Balau/libtomcrypt/src/headers \
llvm/include \
llvm/lib \
llvm-build/include \
llvm-build/lib/Target/X86 \

CPPFLAGS_NO_ARCH += -D__STDC_LIMIT_MACROS -D__STDC_CONSTANT_MACROS

LIBS = z uuid protobuf sqlite3 curses curl

ifeq ($(SYSTEM),Darwin)
    LIBS += pthread iconv
    CONFIG_H = Balau/darwin-config.h
endif

ifeq ($(SYSTEM),Linux)
    LIBS += pthread dl
    CONFIG_H = Balau/linux-config.h
endif

vpath %.cpp src:src/cpu:src/cpu/x86:src/cpu/x86_llvm:src/Loaders/PE:src/Loaders/Elf:src/Loaders/Dmp:src/PDB:wxIgor:src/Loaders/llvm
vpath %.cc src/protobufs
vpath %.proto src/protobufs

IGOR_SOURCES = \
Igor.cpp \
IgorSqlite.cpp \
IgorUtils.cpp \
IgorUsers.cpp \
IgorSession.cpp \
IgorLocalSession.cpp \
IgorDatabase.cpp \
IgorSection.cpp \
IgorAnalysis.cpp \
IgorHttp.cpp \
IgorHttpSession.cpp \
IgorWS.cpp \
\
IgorLLVM.cpp \
\
IgorProto.pb.cc \
\
llvmLoader.cpp\
PELoader.cpp \
elfLoader.cpp \
dmpLoader.cpp \
\
cpuModule.cpp \
cpu_x86_llvm.cpp \
\
gsi.cpp \
msf.cpp \
pdb.cpp \
sym.cpp \
tpi.cpp \

WXIGOR_SOURCES = \
stdafx.cpp \
wxAsmWidget.cpp \
wxIgorApp.cpp \
wxIgorFrame.cpp \
wxManageUsers.cpp \

LLVM_LIBS += \
LLVMAArch64AsmParser LLVMAArch64Disassembler LLVMARMCodeGen LLVMARMAsmParser LLVMARMDisassembler LLVMCppBackendCodeGen \
LLVMHexagonCodeGen LLVMMipsCodeGen LLVMMipsAsmParser LLVMMipsDisassembler LLVMMSP430CodeGen LLVMNVPTXCodeGen LLVMPowerPCCodeGen \
LLVMPowerPCAsmParser LLVMR600CodeGen LLVMSparcCodeGen LLVMSystemZCodeGen LLVMSystemZAsmParser LLVMSystemZDisassembler \
LLVMX86CodeGen LLVMX86AsmParser LLVMX86Disassembler LLVMXCoreCodeGen LLVMXCoreDisassembler LLVMDebugInfo LLVMMCDisassembler \
LLVMAArch64CodeGen LLVMARMDesc LLVMCppBackendInfo LLVMHexagonAsmPrinter LLVMMipsDesc LLVMMSP430Desc LLVMNVPTXDesc \
LLVMPowerPCDesc LLVMR600Desc LLVMSparcDesc LLVMSystemZDesc LLVMX86Desc LLVMXCoreDesc LLVMAArch64Desc LLVMAsmPrinter \
LLVMSelectionDAG LLVMARMAsmPrinter LLVMARMInfo LLVMHexagonDesc LLVMMipsAsmPrinter LLVMMipsInfo LLVMMSP430AsmPrinter \
LLVMMSP430Info LLVMNVPTXAsmPrinter LLVMNVPTXInfo LLVMPowerPCAsmPrinter LLVMPowerPCInfo LLVMR600AsmPrinter LLVMR600Info \
LLVMSparcInfo LLVMSystemZAsmPrinter LLVMSystemZInfo LLVMX86AsmPrinter LLVMX86Info LLVMXCoreAsmPrinter LLVMXCoreInfo \
LLVMAArch64AsmPrinter LLVMAArch64Info LLVMMCParser LLVMCodeGen LLVMHexagonInfo LLVMX86Utils LLVMAArch64Utils LLVMObjCARCOpts \
LLVMScalarOpts LLVMInstCombine LLVMTransformUtils LLVMipa LLVMAnalysis LLVMTarget LLVMCore LLVMMC LLVMObject LLVMSupport \

ifneq (,$(wildcard /usr/include/wx-3.0/wx/wx.h))
    IGOR_SOURCES += $(WXIGOR_SOURCES)
    CPPFLAGS += $(shell wx-config --cppflags) -DUSE_WXWIDGETS
    CPPFLAGS_NO_ARCH += $(shell wx-config --cppflags)
    LDLIBS += $(shell wx-config --libs)
endif

ALL_OBJECTS = $(addsuffix .o, $(notdir $(basename $(IGOR_SOURCES))))
ALL_DEPS = $(addsuffix .dep, $(notdir $(basename $(IGOR_SOURCES))))

CPPFLAGS_NO_ARCH += $(addprefix -I, $(INCLUDES)) -fexceptions -imacros $(CONFIG_H)
CPPFLAGS += $(CPPFLAGS_NO_ARCH) $(ARCH_FLAGS)

LDFLAGS += $(ARCH_FLAGS)
LDLIBS += $(addsuffix .a, $(addprefix llvm-build/lib/lib, $(LLVM_LIBS)))
LDLIBS += $(addprefix -l, $(LIBS))

TARGET=Igor.$(BINEXT)

all: dep Balau llvm $(TARGET)

strip: $(TARGET)
	$(STRIP) $(TARGET)

Balau:
	$(MAKE) -C Balau

llvm:
	mkdir -p llvm-build
	(cd llvm-build ; cmake -DLLVM_REQUIRES_RTTI:BOOL=true -DCMAKE_BUILD_TYPE:STRING=Debug ../llvm)
	REQUIRES_RTTI=1 $(MAKE) -C llvm-build

tests: all
	$(MAKE) -C Balau tests

Balau/libBalau.a:
	$(MAKE) -C Balau

wxIgor/appicon.xpm: wxIgor/igor.ico
	convert wxIgor/igor.ico[0] wxIgor/appicon.xpm
	sed -i 's/static char/static const char/' wxIgor/appicon.xpm

$(TARGET): Balau/libBalau.a $(ALL_OBJECTS)
	$(LD) $(LDFLAGS) -o $@ $(ALL_OBJECTS) ./Balau/libBalau.a ./Balau/LuaJIT/src/libluajit.a ./Balau/libtomcrypt/libtomcrypt.a ./Balau/libtommath/libtommath.a $(LDLIBS)

wxIgorFrame.dep: wxIgor/appicon.xpm

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
	$(MAKE) -C llvm-build clean

deepclean:
	git clean -f -d -x
	git submodule foreach git clean -f -d -x
	git submodule foreach git submodule foreach git clean -f -d -x
	git reset --hard HEAD
	git submodule foreach git reset --hard HEAD
	git submodule foreach git submodule foreach git reset --hard HEAD

.PHONY: clean deepclean strip Balau llvm tests all
