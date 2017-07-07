TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE

#=============================
# Build the pvload support library

INC += pvload.h

LIBRARY_IOC	= pvload
LIB_SRCS	+= pvload.y 
LIB_SRCS	+= pvloadLib.c 
LIB_SRCS	+= iocRegister.c

DBD		= pvload.dbd
#PROD	= pvload pvsave

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

# Extra lex / yacc dependencies (pattern rule doesn't work?)
pvload.o: pvload_lex.c ../pvload.h
