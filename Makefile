## INCLUDES
INCLUDES:=-Isrc/ -Itest/src/ -I./ 

## ERRORS
ERRORS:= -Wextra -W -Wall -Werror -Wl,--no-undefined

## XKLIB_TEST
XKLIB_TEST_DEBUG:=xklib_test.dbg
XKLIB_TEST_RELEASE:=xklib_test.rel

XKLIB_OBJ_DEBUG_OUT:=.od
XKLIB_OBJ_RELEASE_OUT:=.or

## XKLIB
XKLIB_DEBUG:=xklib.dbg
XKLIB_RELEASE:=xklib.rel

CRYPTOPP_LIB:=vendor/cryptopp/libcryptopp.a
DLOPEN_LIB:=
DBGHELP_LIB:=
PTHREAD_LIB:=
DATARACES:=

ifneq (,$(findstring mingw, $(CXX)))
  XKLIB_TEST_DEBUG:=$(XKLIB_TEST_DEBUG).exe
  XKLIB_TEST_RELEASE:=$(XKLIB_TEST_RELEASE).exe
  DBGHELP_LIB:=-ldbghelp -lpsapi 
  DATARACES:=--allow-store-data-races
else
  DLOPEN_LIB:=-ldl
  PTHREAD_LIB:=-lpthread
  DATARACES:=--allow-store-data-races
endif

XKLIB_OBJ_DEBUG=$(subst .cpp,$(XKLIB_OBJ_DEBUG_OUT),$(wildcard src/*.cpp))
XKLIB_OBJ_RELEASE=$(subst .cpp,$(XKLIB_OBJ_RELEASE_OUT),$(wildcard src/*.cpp))

XKLIB_TEST_OBJ_DEBUG=$(subst .cpp,$(XKLIB_OBJ_DEBUG_OUT),$(wildcard test/src/*.cpp))
XKLIB_TEST_OBJ_RELEASE=$(subst .cpp,$(XKLIB_OBJ_RELEASE_OUT),$(wildcard test/src/*.cpp))

XKLIB_DEBUG:=$(XKLIB_DEBUG).a
XKLIB_RELEASE:=$(XKLIB_RELEASE).a

## FLAGS
CPPFLAGS_DEBUG:=-fPIC -std=c++2a -O0 -g $(ERRORS) $(INCLUDES)
CPPFLAGS_RELEASE:=-fPIC -std=c++2a -O3 -pipe $(DATARACES) -frename-registers -fomit-frame-pointer -s $(ERRORS) $(INCLUDES)

## RULES
all: xklib xklib_test

xklib: xklibdbg xklibrel

xklib_test: xklib_testdbg xklib_testrel

xklibdbg: $(XKLIB_DEBUG)
xklibrel: $(XKLIB_RELEASE)

xklib_testdbg: $(XKLIB_TEST_DEBUG)
xklib_testrel: $(XKLIB_TEST_RELEASE) 

cryptopplib:
	@echo 'Compiling cryptopp'
	$(MAKE) -C vendor/cryptopp static 

.PHONY: all clean

$(XKLIB_DEBUG): cryptopplib $(XKLIB_OBJ_DEBUG)
	ar rcs $@ $(XKLIB_OBJ_DEBUG)

$(XKLIB_OBJ_DEBUG): %$(XKLIB_OBJ_DEBUG_OUT): %.cpp
	$(CXX) -c $(CPPFLAGS_DEBUG) $< -o $@

$(XKLIB_RELEASE): cryptopplib $(XKLIB_OBJ_RELEASE)
	ar rcs -o $@ $(XKLIB_OBJ_RELEASE)

$(XKLIB_OBJ_RELEASE): %$(XKLIB_OBJ_RELEASE_OUT): %.cpp
	$(CXX) -c $(CPPFLAGS_RELEASE) $< -o $@

## TEST
$(XKLIB_TEST_DEBUG): cryptopplib $(XKLIB_TEST_OBJ_DEBUG) $(XKLIB_OBJ_DEBUG)
	$(CXX) $(CPPFLAGS_DEBUG) -o $@ $(XKLIB_TEST_OBJ_DEBUG) $(XKLIB_OBJ_DEBUG) $(DLOPEN_LIB) $(DBGHELP_LIB) $(PTHREAD_LIB) $(CRYPTOPP_LIB)

$(XKLIB_TEST_OBJ_DEBUG): %$(XKLIB_OBJ_DEBUG_OUT): %.cpp
	$(CXX) -c $(CPPFLAGS_DEBUG) $< -o $@

$(XKLIB_TEST_RELEASE): cryptopplib $(XKLIB_TEST_OBJ_RELEASE) $(XKLIB_OBJ_RELEASE)
	$(CXX) $(CPPFLAGS_RELEASE) -o $@ $(XKLIB_TEST_OBJ_RELEASE) $(XKLIB_OBJ_RELEASE) $(DLOPEN_LIB) $(DBGHELP_LIB) $(PTHREAD_LIB) $(CRYPTOPP_LIB)

$(XKLIB_TEST_OBJ_RELEASE): %$(XKLIB_OBJ_RELEASE_OUT): %.cpp
	$(CXX) -c $(CPPFLAGS_RELEASE) $< -o $@

clean:
	${RM} $(XKLIB_DEBUG)
	${RM} $(XKLIB_RELEASE)
	${RM} $(XKLIB_OBJ_DEBUG)
	${RM} $(XKLIB_OBJ_RELEASE)
	${RM} $(XKLIB_TEST_DEBUG)
	${RM} $(XKLIB_TEST_RELEASE)
	${RM} $(XKLIB_TEST_OBJ_DEBUG)
	${RM} $(XKLIB_TEST_OBJ_RELEASE)
	$(MAKE) -C vendor/cryptopp clean
