PROGNAME = Brain Visualizer
BINNAME = viz
DEBUGBINNAME = vizd

CXX = g++
LD = $(CXX)
RM = rm -rf

OSDIRNAME = linux
SRCDIR = ../../src
RESDIR = ../../res
CONFIGDIR = ../../config
OBJDIR = ../../tmp/$(OSDIRNAME)
DEBUGOBJDIR = ../../tmp/$(OSDIRNAME)/debug
LIBDIR = ./../lib/$(OSDIRNAME)
BINDIR = ../../bin/$(OSDIRNAME)

CXXFLAGS = -std=c++11 -isystem ../../include -isystem /usr/include -I$(SRCDIR) -I$(RESDIR) -Wall -Wno-unknown-pragmas
LDFLAGS = $(wildcard $(LIBDIR)/*.a) -lm -lpng -lz -lGLU -lGL -lXfixes -lXext -lXft -lfontconfig -lXinerama -lpthread -ldl -lX11 -lXpm

RELEASEFLAGS = -DSHORT_COORDS -DNDEBUG -Ofast -flto -march=native
DEBUGFLAGS = -DDEBUG -D_DEBUG -O0 -g -ggdb3 -Wextra -pedantic -Wsign-conversion
LARGEINTERFACEFLAGS = -DLARGE_INTERFACE
FLOATCOORDFLAGS = -USHORT_COORDS -UINT_COORDS
OPTIMIZELDFLAGS = -Ofast -flto

COMMON = $(wildcard $(SRCDIR)/*.h) $(wildcard $(RESDIR)/*.xpm)
SOURCES = $(wildcard $(SRCDIR)/*.cpp)
OBJECTS = $(SOURCES:$(SRCDIR)/%.cpp=$(OBJDIR)/%.o)
DEBUGOBJECTS = $(SOURCES:$(SRCDIR)/%.cpp=$(DEBUGOBJDIR)/%.o)
TARGET = $(BINDIR)/$(BINNAME)
DEBUGTARGET = $(BINDIR)/$(DEBUGBINNAME)

.PHONY: all $(DEBUGBINNAME) $(BINNAME) debug release large-interface float-coords large-float clean

.SUFFIXES: .o .cpp

all: $(DEBUGBINNAME) $(BINNAME)

$(DEBUGBINNAME): debug
$(BINNAME): release

debug: CXXFLAGS += $(DEBUGFLAGS)
debug: $(DEBUGTARGET)

release: CXXFLAGS += $(RELEASEFLAGS)
release: $(TARGET)

large-interface: CXXFLAGS += $(RELEASEFLAGS)
large-interface: CXXFLAGS += $(LARGEINTERFACEFLAGS)
large-interface: $(TARGET)

float-coords: CXXFLAGS += $(RELEASEFLAGS)
float-coords: CXXFLAGS += $(FLOATCOORDFLAGS)
float-coords: $(TARGET)

large-float: CXXFLAGS += $(RELEASEFLAGS)
large-float: CXXFLAGS += $(LARGEINTERFACEFLAGS) $(FLOATCOORDFLAGS)
large-float: $(TARGET)

$(DEBUGTARGET): $(DEBUGOBJECTS)
	@ mkdir -p $(BINDIR)
	@ echo Linking $@ ...
	@ $(LD) -o $@ $^ $(LDFLAGS)
	@ echo Done building $(PROGNAME)

$(TARGET): $(OBJECTS)
	@ echo Building $(PROGNAME) for release ...
	@ mkdir -p $(BINDIR)
	@ echo Linking $@ ...
	@ $(LD) -o $@ $(OPTIMIZELDFLAGS) $^ $(LDFLAGS)
	@ echo Done building $(PROGNAME)

$(DEBUGOBJDIR)/%.o: $(SRCDIR)/%.cpp $(COMMON)
	@ mkdir -p $(DEBUGOBJDIR)
	@ echo Compiling $@ ...
	@ $(CXX) -c $(CXXFLAGS) -o $@ $<

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp $(COMMON)
	@ mkdir -p $(OBJDIR)
	@ echo Compiling $@ ...
	@ $(CXX) -c $(CXXFLAGS) -o $@ $<

clean:
	@ echo Cleaning ...
	-@ $(RM) $(TARGET) $(DEBUGTARGET) $(OBJECTS) $(DEBUGOBJECTS)
	@ echo Done cleaning
