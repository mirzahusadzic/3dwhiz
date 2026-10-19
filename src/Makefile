include ../include/make.include
include ../include/makegtk.include

WARN = -Wall -Wno-parentheses -Wno-switch -Wconversion -Wno-sign-conversion
OPT = -g -O2

override CPPFLAGS += -I../include


%.o: %.c %.dep Makefile
	$(COMPILE_SHARED) -c $<

%.o: %.cpp %.dep Makefile
	$(COMPILEXX_SHARED) -c $<


.PHONY: all
all: ../lib/lib3dwhiz.so


LIB3DWHIZ_OBJS= gtkrtgextras.o gtkrtgresources.o gtkrtgcamera.o gtkrtgcanvas.o \
	    gtkrtgcanvasgrid.o gtkrtgfourpaned.o gtkrtggizmo.o gtkrtginit.o \
	    gtkrtgmovegizmo.o gtkrtgprimitive.o gtkrtgrotategizmo.o gtkrtgscalegizmo.o \
	    gtkrtgscene.o gtkrtgtext.o gtkrtgtexture.o gtkrtgtexturemap.o gtkrtgtoolbar.o \
	    gtkrtgtoolwindow.o gtkrtgtrimesh.o gtkrtgtripod.o marshalers.o

LIB3DWHIZ_DEPS=$(LIB3DWHIZ_OBJS:.o=.dep)

../lib/lib3dwhiz.so: $(LIB3DWHIZ_OBJS) Makefile
	$(LINKXX_SHARED) $(LIB3DWHIZ_OBJS) -o $@

-include $(LIB3DWHIZ_DEPS)

.PHONY: check
check:

.PHONY: install
install: all
	install -d $(DESTDIR)
	install lib3dwhiz.so $(DESTDIR)/lib3dwhiz.so

.PHONY: clean
clean:
	rm -f ../lib/lib3dwhiz.so $(LIB3DWHIZ_OBJS)

.PHONY: distclean
distclean: clean
	rm -f $(LIB3DWHIZ_DEPS)

FORCE:
