# Makefile for YumaPro project
#  
#   YumaPro ycli directory

############### SOURCE PROFILE #############################
SUBDIR_NM=ycli
SUBDIR_CPP=-I../../../libtecla

############### TARGET PROFILE ##############################
TARGET=$(TBASE)/$(SUBDIR_NM)
BIN_INST=$(TBASE)/bin

####################### MAKE RULES #########################
ifdef DEVELOPER
all:
else
all: yclidummy yclilib
endif  # DEVELOPER

#################### PLATFORM DEFINITIONS ############
include ../platform/platform.profile

################ DEPENDENCIES #########################
# depend rule must be included after the 'all' make rule

include ../platform/platform.profile.depend

test:

install:

uninstall:

yclilib: dependencies $(LBASE)/lib$(SUBDIR_NM).a


# this dummy rule keeps make from deleting the $(OBJS) as
# intermediate files
yclidummy: dependencies $(OBJS)


clean:
	rm -f $(OBJS) $(LBASE)/lib$(SUBDIR_NM).*

superclean:
	rm -f *~  $(DEPS) dependencies $(OBJS) \
	$(LBASE)/lib$(SUBDIR_NM).*

distclean: superclean

.PHONY: yclilib yclidummy

# prevent the make program from choking on all the symbols
# that get generated from autogenerated make rules
.NOEXPORT:

ifndef DEVELOPER
include dependencies
endif
