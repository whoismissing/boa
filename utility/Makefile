#------------------------------------------------------------------------------
.SUFFIXES:
.SUFFIXES: .o .c
.PHONY: clean all depend

LDFLAGS =  -g
CFLAGS = -Os -pipe
DEPEND = ./.depend

#------------------------------------------------------------------------------
# EXTRA_CFLAGS
#------------------------------------------------------------------------------
EXTRA_CFLAGS =
#------------------------------------------------------------------------------
# CFLAGS
#------------------------------------------------------------------------------
DEBUG	= -g -Wall 
IFLAGS 	= -I.
CFLAGS += $(COMMON_CFLAGS) $(EXTRA_CFLAGS) -D_GNU_SOURCE

SOURCES = common_utility.c discovery.c if.c
SOURCES += fastcksum.c

OBJS = $(SOURCES:.c=.o)

#------------------------------------------------------------------------------
# targets
#------------------------------------------------------------------------------
all: depend $(UTILITY_LIB)
$(UTILITY_LIB): $(OBJS)
	$(CC) -s -shared -o libutility.so $(OBJS) -L$(DIR_USERS)/common/mtd-tools -lmtdapi

clean:
	rm -f $(DEPEND)
	rm -f $(OBJS) *.so *.a

depend: $(SOURCES)
	if [ ! -e $(DEPEND) ]; then \
		$(CPP) $(DEBUG) $(CFLAGS) $(IFLAGS) -MM $^ > $(DEPEND); \
	fi
	
-include $(DEPEND)

.c.o:
	$(CC) -c -o $@ -fpic $(DEBUG) $(CFLAGS) $(IFLAGS) $<
