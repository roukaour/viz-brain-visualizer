PROGNAME = Brain Visualizer
BINNAME = viz
DEBUGBINNAME = vizd
OSXAPPNAME = $(PROGNAME).app

CXX = clang++
LD = $(CXX)
RM = rm -rf

OSDIRNAME = osx
SRCDIR = ../../src
RESDIR = ../../res
CONFIGDIR = ../../config
OBJDIR = ../../tmp/$(OSDIRNAME)
MMOBJDIR = $(OBJDIR)/mm
DEBUGOBJDIR = ../../tmp/$(OSDIRNAME)/debug
DEBUGMMOBJDIR = $(DEBUGOBJDIR)/mm
LIBDIR = ../../lib/$(OSDIRNAME)
BINDIR = ../../bin/$(OSDIRNAME)

CXXFLAGS = -std=c++11 --stdlib=libc++ -isystem ../../include -isystem /usr/include -I. -I$(SRCDIR) -I$(RESDIR) -Wall -Wno-unknown-pragmas -Wno-deprecated-declarations
LDFLAGS = $(wildcard $(LIBDIR)/*.a) -lm -lz -framework AGL -framework OpenGL -framework ApplicationServices -lpthread -framework Cocoa

RELEASEFLAGS = -DNDEBUG -Ofast -flto -DSHORT_COORDS
DEBUGFLAGS = -DDEBUG -D_DEBUG -O0 -g -ggdb3 -Wextra -pedantic -Wsign-conversion
LARGEINTERFACEFLAGS = -DLARGE_INTERFACE
FLOATCOORDFLAGS = -USHORT_COORDS -UINT_COORDS
OPTIMIZELDFLAGS = -Ofast -flto

COMMON = $(wildcard $(SRCDIR)/*.h) $(wildcard $(RESDIR)/*.xpm)
SOURCES = $(wildcard $(SRCDIR)/*.cpp)
MMSOURCES = $(wildcard ./*.mm)
OBJECTS = $(SOURCES:$(SRCDIR)/%.cpp=$(OBJDIR)/%.o) $(MMSOURCES:%.mm=$(MMOBJDIR)/%.o)
DEBUGOBJECTS = $(SOURCES:$(SRCDIR)/%.cpp=$(DEBUGOBJDIR)/%.o) $(MMSOURCES:%.mm=$(DEBUGMMOBJDIR)/%.o)
TARGET = $(BINDIR)/$(BINNAME)
DEBUGTARGET = $(BINDIR)/$(DEBUGBINNAME)
OSXAPPTARGET = $(BINDIR)/$(OSXAPPNAME)
OSXINFOPLIST = $(OBJDIR)/Info.plist

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
	@ echo Building $(OSXINFOPLIST) ...
	@ $(CXX) -E -x c -P -I$(SRCDIR) -o $(OSXINFOPLIST) Info.plist.template
	@ echo Building $(PROGNAME) for release ...
	@ mkdir -p $(BINDIR)
	@ echo Linking $@ ...
	@ $(LD) -o $@ $(OPTIMIZELDFLAGS) $^ $(LDFLAGS)
	@ echo Creating app bundle \"$(OSXAPPTARGET)\" ...
	@ mkdir -p "$(OSXAPPTARGET)/Contents/Resources"
	@ mkdir -p "$(OSXAPPTARGET)/Contents/MacOS"
	@ echo APPLnone > "$(OSXAPPTARGET)/Contents/PkgInfo"
	@ echo "#!/bin/bash\ncd \"\$$(dirname \"\$${BASH_SOURCE[0]}\")/../../..\"\n\"./$(OSXAPPNAME)/Contents/Resources/$(PROGNAME)\"" > "$(OSXAPPTARGET)/Contents/MacOS/launchviz"
	@ chmod 755 "$(OSXAPPTARGET)/Contents/MacOS/launchviz"
	@ cp $@ "$(OSXAPPTARGET)/Contents/Resources/$(PROGNAME)"
	@ chmod 755 "$(OSXAPPTARGET)/Contents/Resources/$(PROGNAME)"
	@ cp "$(OSXINFOPLIST)" "$(OSXAPPTARGET)/Contents/Info.plist"
	@ cp "$(RESDIR)/viz.icns" "$(OSXAPPTARGET)/Contents/Resources/viz.icns"
	@ cp "$(CONFIGDIR)/help.html" "$(OSXAPPTARGET)/Contents/Resources/help.html"
	@ echo Done building $(PROGNAME)

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp $(COMMON)
	@ mkdir -p $(OBJDIR)
	@ echo Compiling $@ ...
	@ $(CXX) -c $(CXXFLAGS) -o $@ $<

$(DEBUGOBJDIR)/%.o: $(SRCDIR)/%.cpp $(COMMON)
	@ mkdir -p $(DEBUGOBJDIR)
	@ echo Compiling $@ ...
	@ $(CXX) -c $(CXXFLAGS) -o $@ $<

$(MMOBJDIR)/%.o: %.mm $(COMMON)
	@ mkdir -p $(MMOBJDIR)
	@ echo Compiling $@ as Objective-C ...
	@ $(CXX) -c $(CXXFLAGS) -o $@ $<

$(DEBUGMMOBJDIR)/%.o: %.mm $(COMMON)
	@ mkdir -p $(DEBUGMMOBJDIR)
	@ echo Compiling $@ as Objective-C ...
	@ $(CXX) -c $(CXXFLAGS) -o $@ $<

clean:
	@ echo Cleaning ...
	-@ $(RM) $(TARGET) $(DEBUGTARGET) "$(OSXAPPTARGET)" "$(OSXINFOPLIST)" $(OBJECTS) $(DEBUGOBJECTS)
	@ echo Done cleaning
