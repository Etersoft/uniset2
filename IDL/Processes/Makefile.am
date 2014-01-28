############################################################################
# This file is part of the UniSet library                                  #
############################################################################

# Каталоги для размещения генерируемых CC- и HH-файлов соответственно
CCDIR=$(top_builddir)/src/Processes
HHDIR=$(top_builddir)/include

# Исходные файлы IDL
IDLFILES=IOController_i.idl

include $(top_builddir)/IDL/idl.mk

idl_include_HEADERS = *.idl
idl_includedir = $(datadir)/idl/@PACKAGE@/Processes
