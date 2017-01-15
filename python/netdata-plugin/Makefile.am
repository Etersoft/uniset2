if DISABLE_PYTHON
else
if HAVE_NETDATA
netdataconfdir = $(sysconfdir)/netdata/python.d
netdataconf_DATA = *.conf

plugdir = $(libdir)/netdata/python.d
plug_DATA = *.py

include $(top_builddir)/include.mk

endif
endif
