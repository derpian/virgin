EXTRA_DIST = \
		Makefile.mingw \
		wincred.c
CLEANFILES =

plugindir = @PURPLE_PLUGINDIR@

internalkeyring_la_CFLAGS  = $(AM_CPPFLAGS)
internalkeyring_la_LDFLAGS = -module @PLUGIN_LDFLAGS@
internalkeyring_la_SOURCES = internalkeyring.c
internalkeyring_la_LIBADD  = @PURPLE_LIBS@

if ENABLE_SECRETSERVICE

secretservice_la_CFLAGS  = $(AM_CPPFLAGS) $(SECRETSERVICE_CFLAGS)
secretservice_la_LDFLAGS = -module @PLUGIN_LDFLAGS@
secretservice_la_SOURCES = secretservice.c
secretservice_la_LIBADD  = @PURPLE_LIBS@ $(SECRETSERVICE_LIBS)

endif

if ENABLE_GNOMEKEYRING

gnomekeyring_la_CFLAGS  = $(AM_CPPFLAGS) $(GNOMEKEYRING_CFLAGS)
gnomekeyring_la_LDFLAGS = -module @PLUGIN_LDFLAGS@
gnomekeyring_la_SOURCES = gnomekeyring.c
gnomekeyring_la_LIBADD  = @PURPLE_LIBS@ $(GNOMEKEYRING_LIBS)

endif

if IS_WIN32

wincred_la_CFLAGS  = $(AM_CPPFLAGS)
wincred_la_LDFLAGS = -module @PLUGIN_LDFLAGS@
wincred_la_SOURCES = wincred.c
wincred_la_LIBADD  = @PURPLE_LIBS@

endif

if ENABLE_KWALLET

kwallet_la_CXXFLAGS  = $(KWALLET_CXXFLAGS) $(QT4_CFLAGS)
kwallet_la_LDFLAGS = -module @PLUGIN_LDFLAGS@
kwallet_la_SOURCES = kwallet.cpp
kwallet_la_BUILTSOURCES = kwallet.moc
kwallet_la_LIBADD  = @PURPLE_LIBS@ $(KWALLET_LIBS) $(QT4_LIBS)

kwallet.cpp: kwallet.moc

kwallet.moc:
	$(AM_V_GEN)$(MOC) $(kwallet_la_CXXFLAGS) -i $(srcdir)/kwallet.cpp -o $@

CLEANFILES += kwallet.moc

endif

if PLUGINS

plugin_LTLIBRARIES = \
	internalkeyring.la

if ENABLE_SECRETSERVICE
plugin_LTLIBRARIES += \
	secretservice.la
endif

if ENABLE_GNOMEKEYRING
plugin_LTLIBRARIES += \
	gnomekeyring.la
endif

if IS_WIN32
plugin_LTLIBRARIES += \
	wincred.la
endif

if ENABLE_KWALLET
plugin_LTLIBRARIES += \
	kwallet.la
endif

endif

#XXX: that might be done better than adding DEBUG_CPPFLAGS to all objects (not only C++ ones)
AM_CPPFLAGS = \
	-I$(top_srcdir)/libpurple \
	-I$(top_builddir)/libpurple \
	$(GLIB_CFLAGS) \
	$(GPLUGIN_CFLAGS) \
	$(DEBUG_CPPFLAGS) \
	$(PLUGIN_CFLAGS)
