############################################################################
# This file is part of the UniSet library								  #
############################################################################

# Каталоги для размещения генерируемых CC- и HH-файлов соответственно
CCDIR=$(top_builddir)/src/ObjectRepository
HHDIR=$(top_builddir)/include

# Исходные файлы IDL
IDLFILES=UniSetBaseConstants.idl UniSetTypes_i.idl UniSetObject_i.idl UniSetManager_i.idl

include $(top_builddir)/IDL/idl.mk

idl_include_HEADERS = *.idl
idl_includedir = $(datadir)/idl/@PACKAGE@/UniSetTypes

include $(top_builddir)/include.mk
