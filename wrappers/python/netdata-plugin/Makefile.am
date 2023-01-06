if ENABLE_PYTHON
if HAVE_NETDATA
netdataconfdir = $(sysconfdir)/netdata/python.d
netdataconf_DATA = uniset.conf

plugdir = $(libdir)/netdata/python.d
plug_DATA = $(wildcard $(srcdir)/*.py)

include $(top_builddir)/include.mk

endif
endif
