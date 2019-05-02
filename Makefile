#####################################################
#  Makefile AOR_SERVER 
#
#####################################################

#
# Flags 
#
CC = gcc
#CFLAGS  = -g -Wall $(INCLUDE) -DAOR_SERVER_ -D_DEBUG
CFLAGS  = -Wall -O2 $(INCLUDE) -DAOR_SERVER_
AR = ar
ARFLAGS = -r

#
# Exec or Libs
# 
LIB_PROG = ./lib/
DIR_PROG = ./bin/
PROG     = $(DIR_PROG)aor_server

#
# Headers
#
INCLUDE = \
          -I.

#
# Libs
LDLIBS    = -L$(DIR_PROG) -L$(LIB_PROG) -lpthread -lm -ljsmn -lstdc++

#
# Directory
#
DIR_OBJS = ./.obj/

OBJS  = \
        $(DIR_OBJS)aor_server.o


#####################################################

#
# targets
#
.PHONY: all
all: criaObjDir $(PROG) fim

#
# Redefinition
#
$(DIR_OBJS)%.o : %.c
	$(CC) -c $(CFLAGS) $< -o $@

#
# Redefinition 
#
$(PROG): $(OBJS)
	$(CC) $< $(LDLIBS) -o $@
#	$(AR) $(ARFLAGS) $(PROG) $(OBJS)



.PHONY: criaObjDir
criaObjDir:
	-@echo "#####################################################";
	-@echo "making $(PROG)"
	-@echo ""
	-@if [ ! -d $(DIR_OBJS) ] ; \
	then \
	  echo "" ; \
	  echo "Diretory of objects does not exist !!" ; \
	  echo "Creating it" ; \
	  mkdir $(DIR_OBJS) ; \
	  echo "" ; \
	fi

.PHONY: fim
fim:
	@echo ""

.PHONY: clean
clean:
	-rm -f $(OBJS) $(PROG)

.PHONY: cleanall
cleanall: clean
	-rm -f core *.core *~
	-@if [ -d $(DIR_OBJS) ] ; \
	then \
	  echo "Removing diretory $(DIR_OBJS)" ; \
	  rmdir $(DIR_OBJS) ; \
	  echo "" ; \
	fi
