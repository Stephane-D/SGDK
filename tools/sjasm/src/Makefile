ISMINGW := $(shell uname | grep MINGW)
ifneq ($(ISMINGW),)
	EXTENSION = .exe
else
	EXTENSION = 
endif

TARGETS	 = sjasm$(EXTENSION)
OBJECTS  = direct.o loose.o parser.o piz80.o reader.o sjasm.o sjio.o tables.o 

BINDIR   = /usr/local/bin

CXXFLAGS = -Wall -O2
ifneq ($(ISMINGW),)
	CXXFLAGS += -DMINGW
else
	CXXFLAGS += -DMAX_PATH=MAXPATHLEN
endif
LDFLAGS  = 

DEPDIR = .deps
DEPFILE = $(DEPDIR)/$(*F)

all: $(TARGETS)

sjasm$(EXTENSION): $(DEPDIR) $(OBJECTS)
	g++ $(LDFLAGS) -o $@ $(OBJECTS) 
	strip $@

clean:
	$(RM) $(OBJECTS) $(TARGETS)
	$(RM) -r $(DEPDIR)	

install: all
	cp -f $(TARGETS) $(BINDIR)

%.o : %.cpp
	$(CXX) -Wp,-MD,$(DEPFILE).d $(CXXFLAGS) -c -o $@ $<
	@cp $(DEPFILE).d $(DEPFILE).P; \
		sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
		-e '/^$$/ d' -e 's/$$/ :/' < $(DEPFILE).d >> $(DEPFILE).P; \
	rm -f $(DEPFILE).d

$(DEPDIR):
	@mkdir $(DEPDIR)
	
-include $(OBJECTS:%.o=$(DEPDIR)/%.P)

