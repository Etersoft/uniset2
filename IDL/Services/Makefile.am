############################################################################
# This file is part of the UniSet library                                  #
############################################################################

# Каталоги для размещения генерируемых CC- и HH-файлов соответственно
CCDIR=$(top_builddir)/src/Services
HHDIR=$(top_builddir)/include

# Исходные файлы IDL
IDLFILES=TimerService_i.idl InfoServer_i.idl

include $(top_builddir)/conf/idl.mk

idl_include_HEADERS = *.idl
idl_includedir = $(datadir)/idl/@PACKAGE@/Services
