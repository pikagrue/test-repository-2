# Makefile for libtoaster entire directory
#  
#   very top libtoaster directory

# BUILD VARIABLES
#
# DEVELOPER=1 : USED FOR debian yuma-dev package

##################### LIBRARIES ########################
#
# DIRS:  
# This is an ordered list
# Put only the directories that build object files in the list

# make all targets
DIRS = src


ifndef PREFIX
    PREFIX=/usr
endif

SRC_INST=$(DESTDIR)$(PREFIX)/share/yumapro/src/libtoaster


#################### PLATFORM DEFINITIONS ############
all: toasterall


################################ MAKE RULES #################################
#

clean: toasterclean

superclean: toastersuperclean

distclean: toastersuperclean

uninstall: toasterclean
	rm -rf $(SRC_INST)

ifdef DEBIAN
 ifdef DEVELOPER
install: srcinstall
 endif # DEVELOPER

 ifdef EVAL
install: libinstall srcinstall
 endif # EVAL

 ifdef SERVER
install: libinstall srcinstall
 endif # SERVER

 ifdef BUILD_ALL
install: libinstall srcinstall
 endif # BUILD_ALL

else # not DEBIAN
install: libinstall srcinstall
endif  # DEBIAN

toasterall:
	mkdir -p lib bin
	for dir in $(DIRS); do\
	  cd $$dir && $(MAKE) && cd ..;\
        done

toasterclean:
	for dir in $(DIRS); do\
	  cd $$dir && $(MAKE) clean && cd ..;\
        done

toastersuperclean:
	for dir in $(DIRS); do\
	  cd $$dir && $(MAKE) superclean && cd ..;\
        done

libinstall:
	for dir in $(DIRS); do\
          cd $$dir && $(MAKE) install && cd ..;\
        done

srcinstall:
	mkdir -p $(SRC_INST)
	mkdir -p $(SRC_INST)/src
	mkdir -p $(SRC_INST)/lib
	mkdir -p $(SRC_INST)/bin
	cp Makefile $(SRC_INST)
	cp src/*start $(SRC_INST)/src/
	cp src/*c $(SRC_INST)/src/
	cp src/*h $(SRC_INST)/src/
	cp src/Makefile $(SRC_INST)/src/


.PHONY: all clean superclean install libinstall \
	toasterall toasterclean toastersuperclean srcinstall distclean


# prevent the make program from choking on all the symbols
# that get generated from autogenerated make rules
.NOEXPORT:



