#ifneq ($(PREFIX),)
	PREFIX := /usr/local
#endif

# XLIB_TEST
XLIB_TEST_DEBUG:=xlib_test.dbg
XLIB_TEST_RELEASE:=xlib_test.rel

XLIB_OBJ_DEBUG_OUT:=.od
XLIB_OBJ_RELEASE_OUT:=.or

# XLIB
XLIB_DEBUG:=xlib.dbg
XLIB_RELEASE:=xlib.rel

# CRYPTOPP
CRYPTOPP_LIB:=cryptopp
DLOPEN_LIB:=
DBGHELP_LIB:=

# Let's see if it's asked us to compile in 32 bits.
ifneq (,$(findstring -m32, $(CXX)))
	CRYPTOPP_LIB:=$(CRYPTOPP_LIB)32
	XLIB_DEBUG:=$(XLIB_DEBUG)32
	XLIB_RELEASE:=$(XLIB_RELEASE)32
	XLIB_TEST_DEBUG:=$(XLIB_TEST_DEBUG)32
	XLIB_TEST_RELEASE:=$(XLIB_TEST_RELEASE)32
	XLIB_OBJ_DEBUG_OUT:=$(XLIB_OBJ_DEBUG_OUT)32
	XLIB_OBJ_RELEASE_OUT:=$(XLIB_OBJ_RELEASE_OUT)32
else ifneq (,$(findstring i686, $(CXX)))
	CRYPTOPP_LIB:=$(CRYPTOPP_LIB)32
	XLIB_DEBUG:=$(XLIB_DEBUG)32
	XLIB_RELEASE:=$(XLIB_RELEASE)32
	XLIB_TEST_DEBUG:=$(XLIB_TEST_DEBUG)32
	XLIB_TEST_RELEASE:=$(XLIB_TEST_RELEASE)32
	XLIB_OBJ_DEBUG_OUT:=$(XLIB_OBJ_DEBUG_OUT)32
	XLIB_OBJ_RELEASE_OUT:=$(XLIB_OBJ_RELEASE_OUT)32
else ifneq (,$(findstring i386, $(CXX)))
	CRYPTOPP_LIB:=$(CRYPTOPP_LIB)32
	XLIB_DEBUG:=$(XLIB_DEBUG)32
	XLIB_RELEASE:=$(XLIB_RELEASE)32
	XLIB_TEST_DEBUG:=$(XLIB_TEST_DEBUG)32
	XLIB_TEST_RELEASE:=$(XLIB_TEST_RELEASE)32
	XLIB_OBJ_DEBUG_OUT:=$(XLIB_OBJ_DEBUG_OUT)32
	XLIB_OBJ_RELEASE_OUT:=$(XLIB_OBJ_RELEASE_OUT)32
else
	CRYPTOPP_LIB:=$(CRYPTOPP_LIB)
	XLIB_DEBUG:=$(XLIB_DEBUG)64
	XLIB_RELEASE:=$(XLIB_RELEASE)64
	XLIB_TEST_DEBUG:=$(XLIB_TEST_DEBUG)64
	XLIB_TEST_RELEASE:=$(XLIB_TEST_RELEASE)64
	XLIB_OBJ_DEBUG_OUT:=$(XLIB_OBJ_DEBUG_OUT)64
	XLIB_OBJ_RELEASE_OUT:=$(XLIB_OBJ_RELEASE_OUT)64
endif

ifneq (,$(findstring mingw, $(CXX)))
	CRYPTOPP_LIB:=$(CRYPTOPP_LIB)win
	XLIB_DEBUG:=$(XLIB_DEBUG).win
	XLIB_RELEASE:=$(XLIB_RELEASE).win
	XLIB_TEST_DEBUG:=$(XLIB_TEST_DEBUG).exe
	XLIB_TEST_RELEASE:=$(XLIB_TEST_RELEASE).exe
	XLIB_OBJ_DEBUG_OUT:=$(XLIB_OBJ_DEBUG_OUT)win
	XLIB_OBJ_RELEASE_OUT:=$(XLIB_OBJ_RELEASE_OUT)win
	DBGHELP_LIB:=-ldbghelp
else
	DLOPEN_LIB :=-ldl
endif

XLIB_OBJ_DEBUG=$(subst .cpp,$(XLIB_OBJ_DEBUG_OUT),$(wildcard src/*.cpp))
XLIB_OBJ_RELEASE=$(subst .cpp,$(XLIB_OBJ_RELEASE_OUT),$(wildcard src/*.cpp))

XLIB_TEST_OBJ_DEBUG=$(subst .cpp,$(XLIB_OBJ_DEBUG_OUT),$(wildcard test/src/*.cpp))
XLIB_TEST_OBJ_RELEASE=$(subst .cpp,$(XLIB_OBJ_RELEASE_OUT),$(wildcard test/src/*.cpp))

XLIB_DEBUG:=$(XLIB_DEBUG).a
XLIB_RELEASE:=$(XLIB_RELEASE).a

CPPFLAGS_DEBUG:= -static-libstdc++ -static-libgcc -std=c++2a -O0 -g -Wextra -W -Wall -Werror -Wl,--no-undefined -Isrc/ -Itest/src/ -I$(PREFIX)/include/ -L$(PREFIX)/lib/

CPPFLAGS_RELEASE:= -static-libstdc++ -static-libgcc -std=c++2a -Ofast -pipe --allow-store-data-races -frename-registers -fomit-frame-pointer -s -Wextra -W -Wall -Werror -Wl,--no-undefined -Isrc/ -Itest/src/ -I$(PREFIX)/include/ -L$(PREFIX)/lib/

all: xlib xlib_test

xlib: xlibdbg xlibrel
xlib_test: xlib_testdbg xlib_testrel

xlibdbg: $(XLIB_DEBUG)
xlibrel: $(XLIB_RELEASE)

xlib_testdbg: $(XLIB_TEST_DEBUG)
xlib_testrel: $(XLIB_TEST_RELEASE)

install:
		# Static libraries.
		install -d $(DESTDIR)$(PREFIX)/lib/
		install -d $(DESTDIR)$(PREFIX)/lib/xlib/
		install -m 4 $(XLIB_DEBUG) $(DESTDIR)$(PREFIX)/lib/xlib/
		install -m 4 $(XLIB_DEBUG) $(DESTDIR)$(PREFIX)/lib/xlib/
		install -m 4 $(XLIB_RELEASE) $(DESTDIR)$(PREFIX)/lib/xlib/
		install -m 4 $(XLIB_RELEASE) $(DESTDIR)$(PREFIX)/lib/xlib/

		# Headers.
		install -d $(DESTDIR)$(PREFIX)/include/
		install -d $(DESTDIR)$(PREFIX)/include/xlib/
		cp ./src/*.h $(DESTDIR)$(PREFIX)/include/xlib/


.PHONY: all clean

$(XLIB_DEBUG): $(XLIB_OBJ_DEBUG)
				ar rcs $@ $^

$(XLIB_OBJ_DEBUG): %$(XLIB_OBJ_DEBUG_OUT): %.cpp
				$(CXX) -c $(CPPFLAGS_DEBUG) $< -o $@

$(XLIB_RELEASE): $(XLIB_OBJ_RELEASE)
				ar rcs -o $@ $^

$(XLIB_OBJ_RELEASE): %$(XLIB_OBJ_RELEASE_OUT): %.cpp
				$(CXX) -c $(CPPFLAGS_RELEASE) $< -o $@

# TEST

$(XLIB_TEST_DEBUG): $(XLIB_TEST_OBJ_DEBUG) $(XLIB_OBJ_DEBUG)
				$(CXX) $(CPPFLAGS_DEBUG) -o $@ $^ -l$(CRYPTOPP_LIB) $(DLOPEN_LIB) $(DBGHELP_LIB)

$(XLIB_TEST_OBJ_DEBUG): %$(XLIB_OBJ_DEBUG_OUT): %.cpp
				$(CXX) -c $(CPPFLAGS_DEBUG) $< -o $@

$(XLIB_TEST_RELEASE): $(XLIB_TEST_OBJ_RELEASE) $(XLIB_OBJ_RELEASE)
				$(CXX) $(CPPFLAGS_RELEASE) -o $@ $^ -l$(CRYPTOPP_LIB) $(DLOPEN_LIB) $(DBGHELP_LIB)

$(XLIB_TEST_OBJ_RELEASE): %$(XLIB_OBJ_RELEASE_OUT): %.cpp
				$(CXX) -c $(CPPFLAGS_RELEASE) $< -o $@

clean:
				${RM} $(XLIB_DEBUG)
				${RM} $(XLIB_RELEASE)
				${RM} $(XLIB_OBJ_DEBUG)
				${RM} $(XLIB_OBJ_RELEASE)
				${RM} $(XLIB_TEST_DEBUG)
				${RM} $(XLIB_TEST_RELEASE)
				${RM} $(XLIB_TEST_OBJ_DEBUG)
				${RM} $(XLIB_TEST_OBJ_RELEASE)
