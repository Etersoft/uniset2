############################################################################
# This file is part of the UniSet library                                  #
############################################################################

# Каталоги для размещения генерируемых CC- и HH-файлов соответственно
CCDIR=$(top_builddir)/src/ObjectRepository
HHDIR=$(top_builddir)/include

# Исходные файлы IDL
IDLFILES=UniSetTypes_i.idl UniSetObject_i.idl ObjectsManager_i.idl

include $(top_builddir)/conf/idl.mk

idl_include_HEADERS = *.idl
idl_includedir = $(datadir)/idl/@PACKAGE@/UniSetTypes
