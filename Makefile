current_dir:=$(shell pwd)

## INCLUDES
INCLUDES:=-Isrc/ -Itest/src/

## ERRORS
ERRORS:= -Wextra -W -Wall -Werror -Wl,--no-undefined

## XLIB_TEST
XLIB_TEST_DEBUG:=xlib_test.dbg
XLIB_TEST_RELEASE:=xlib_test.rel

XLIB_OBJ_DEBUG_OUT:=.od
XLIB_OBJ_RELEASE_OUT:=.or

## XLIB
XLIB_DEBUG:=xlib.dbg
XLIB_RELEASE:=xlib.rel

CRYPTOPP_LIB:=$(current_dir)/vendor/cryptopp/libcryptopp.a
DLOPEN_LIB:=
DBGHELP_LIB:=
PTHREAD_LIB:=
DATARACES:=

ifneq (,$(findstring mingw, $(CXX)))
  XLIB_TEST_DEBUG:=$(XLIB_TEST_DEBUG).exe
  XLIB_TEST_RELEASE:=$(XLIB_TEST_RELEASE).exe
  DBGHELP_LIB:=-ldbghelp -lpsapi 
  DATARACES:=--allow-store-data-races
else
  DLOPEN_LIB:=-ldl
  PTHREAD_LIB:=-lpthread
  DATARACES:=--allow-store-data-races
endif

XLIB_OBJ_DEBUG=$(subst .cpp,$(XLIB_OBJ_DEBUG_OUT),$(wildcard src/*.cpp))
XLIB_OBJ_RELEASE=$(subst .cpp,$(XLIB_OBJ_RELEASE_OUT),$(wildcard src/*.cpp))

XLIB_TEST_OBJ_DEBUG=$(subst .cpp,$(XLIB_OBJ_DEBUG_OUT),$(wildcard test/src/*.cpp))
XLIB_TEST_OBJ_RELEASE=$(subst .cpp,$(XLIB_OBJ_RELEASE_OUT),$(wildcard test/src/*.cpp))

XLIB_DEBUG:=$(XLIB_DEBUG).a
XLIB_RELEASE:=$(XLIB_RELEASE).a

## FLAGS
CPPFLAGS_DEBUG:=-fPIC -std=c++2a -O0 -g $(ERRORS) $(INCLUDES)
CPPFLAGS_RELEASE:=-fPIC -std=c++2a -O3 -pipe $(DATARACES) -frename-registers -fomit-frame-pointer -s $(ERRORS) $(INCLUDES)

## RULES
all: xlib xlib_test

xlib: xlibdbg xlibrel

xlib_test: xlib_testdbg xlib_testrel

xlibdbg: $(XLIB_DEBUG)
xlibrel: $(XLIB_RELEASE)

xlib_testdbg: $(XLIB_TEST_DEBUG)
xlib_testrel: $(XLIB_TEST_RELEASE) 

cryptopplib:
	@echo 'Compiling cryptopp'
	$(MAKE) -C $(current_dir)/vendor/cryptopp 

.PHONY: all clean

$(XLIB_DEBUG): cryptopplib $(XLIB_OBJ_DEBUG)
	ar rcs $@ $(XLIB_OBJ_DEBUG)

$(XLIB_OBJ_DEBUG): %$(XLIB_OBJ_DEBUG_OUT): %.cpp
	$(CXX) -c $(CPPFLAGS_DEBUG) $< -o $@

$(XLIB_RELEASE): cryptopplib $(XLIB_OBJ_RELEASE)
	ar rcs -o $@ $(XLIB_OBJ_RELEASE)

$(XLIB_OBJ_RELEASE): %$(XLIB_OBJ_RELEASE_OUT): %.cpp
	$(CXX) -c $(CPPFLAGS_RELEASE) $< -o $@

## TEST
$(XLIB_TEST_DEBUG): cryptopplib $(XLIB_TEST_OBJ_DEBUG) $(XLIB_OBJ_DEBUG)
	$(CXX) $(CPPFLAGS_DEBUG) -o $@ $(XLIB_TEST_OBJ_DEBUG) $(XLIB_OBJ_DEBUG) $(DLOPEN_LIB) $(DBGHELP_LIB) $(PTHREAD_LIB) $(CRYPTOPP_LIB)

$(XLIB_TEST_OBJ_DEBUG): %$(XLIB_OBJ_DEBUG_OUT): %.cpp
	$(CXX) -c $(CPPFLAGS_DEBUG) $< -o $@

$(XLIB_TEST_RELEASE): cryptopplib $(XLIB_TEST_OBJ_RELEASE) $(XLIB_OBJ_RELEASE)
	$(CXX) $(CPPFLAGS_RELEASE) -o $@ $(XLIB_TEST_OBJ_RELEASE) $(XLIB_OBJ_RELEASE) $(DLOPEN_LIB) $(DBGHELP_LIB) $(PTHREAD_LIB) $(CRYPTOPP_LIB)

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
	$(MAKE) -C $(current_dir)/vendor/cryptopp clean
