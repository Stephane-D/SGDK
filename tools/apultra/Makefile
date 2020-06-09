CC=gcc
CFLAGS=-O3 -g -fomit-frame-pointer -Isrc/libdivsufsort/include -Isrc
OBJDIR=obj
LDFLAGS=

$(OBJDIR)/%.o: src/../%.c
	@mkdir -p '$(@D)'
	$(CC) $(CFLAGS) -c $< -o $@

APP := apultra

OBJS += $(OBJDIR)/src/appack_compat.o
OBJS += $(OBJDIR)/src/expand.o
OBJS += $(OBJDIR)/src/matchfinder.o
OBJS += $(OBJDIR)/src/shrink.o
OBJS += $(OBJDIR)/src/libdivsufsort/lib/divsufsort.o
OBJS += $(OBJDIR)/src/libdivsufsort/lib/divsufsort_utils.o
OBJS += $(OBJDIR)/src/libdivsufsort/lib/sssort.o
OBJS += $(OBJDIR)/src/libdivsufsort/lib/trsort.o

all: $(APP)

$(APP): $(OBJS)
	$(CC) $^ $(LDFLAGS) -o $(APP)

clean:
	@rm -rf $(APP) $(OBJDIR)

