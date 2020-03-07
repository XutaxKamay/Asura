ifeq ($(PREFIX),)
		PREFIX := /usr/local
endif

# XLIB_TEST
XLIB_TEST32_DEBUG=x_test32.dbg
XLIB_TEST64_DEBUG=x_test64.dbg
XLIB_TEST32_RELEASE=x_test32.rel
XLIB_TEST64_RELEASE=x_test64.rel

XLIB_TEST_OBJ32_DEBUG=$(subst .cpp,.o32d,$(wildcard test/src/*.cpp))
XLIB_TEST_OBJ64_DEBUG=$(subst .cpp,.o64d,$(wildcard test/src/*.cpp))
XLIB_TEST_OBJ32_RELEASE=$(subst .cpp,.o32r,$(wildcard test/src/*.cpp))
XLIB_TEST_OBJ64_RELEASE=$(subst .cpp,.o64r,$(wildcard test/src/*.cpp))

# XLIB
XLIB32_DEBUG=xlib32.dbg.a
XLIB64_DEBUG=xlib64.dbg.a
XLIB32_RELEASE=xlib32.rel.a
XLIB64_RELEASE=xlib64.rel.a

XLIB_OBJ32_DEBUG=$(subst .cpp,.o32d,$(wildcard src/*.cpp))
XLIB_OBJ64_DEBUG=$(subst .cpp,.o64d,$(wildcard src/*.cpp))
XLIB_OBJ32_RELEASE=$(subst .cpp,.o32r,$(wildcard src/*.cpp))
XLIB_OBJ64_RELEASE=$(subst .cpp,.o64r,$(wildcard src/*.cpp))


CPPFLAGS32_DEBUG=-m32 -std=c++17 -g -Wextra -W -Wall -Werror -Wl,--no-undefined -Iinclude/ -Itest/include/

CPPFLAGS64_DEBUG=-m64 -std=c++17 -g -Wextra -W -Wall -Werror -Wl,--no-undefined -Iinclude/ -Itest/include/

CPPFLAGS32_RELEASE=-m32 -std=c++17 -s -Wextra -W -Wall -Werror -Wl,--no-undefined -Iinclude/ -Itest/include/

CPPFLAGS64_RELEASE=-m64 -std=c++17 -s -Wextra -W -Wall -Werror -Wl,--no-undefined -Iinclude/ -Itest/include/

all: xlib xlib_test

xlib: xlib32dbg xlib64dbg xlib32rel xlib64rel
xlib_test: xlib_test32dbg xlib_test64dbg xlib_test32rel xlib_test64rel

xlib32dbg: $(XLIB32_DEBUG)
xlib64dbg: $(XLIB64_DEBUG)
xlib32rel: $(XLIB32_RELEASE)
xlib64rel: $(XLIB64_RELEASE)

xlib_test32dbg: $(XLIB_TEST32_DEBUG)
xlib_test64dbg: $(XLIB_TEST64_DEBUG)
xlib_test32rel: $(XLIB_TEST32_RELEASE)
xlib_test64rel: $(XLIB_TEST64_RELEASE)

install:
		# Static libraries.
		install -d $(DESTDIR)$(PREFIX)/lib/
		install -d $(DESTDIR)$(PREFIX)/lib/xlib/
		install -m 644 $(XLIB32_DEBUG) $(DESTDIR)$(PREFIX)/lib/xlib/
		install -m 644 $(XLIB64_DEBUG) $(DESTDIR)$(PREFIX)/lib/xlib/
		install -m 644 $(XLIB32_RELEASE) $(DESTDIR)$(PREFIX)/lib/xlib/
		install -m 644 $(XLIB64_RELEASE) $(DESTDIR)$(PREFIX)/lib/xlib/

		# Headers.
		install -d $(DESTDIR)$(PREFIX)/include/
		install -d $(DESTDIR)$(PREFIX)/include/xlib/
		cp -r ./include/*.h $(DESTDIR)$(PREFIX)/include/xlib/


.PHONY: all clean

$(XLIB32_DEBUG): $(XLIB_OBJ32_DEBUG)
				ar rcs $@ $^

$(XLIB_OBJ32_DEBUG): %.o32d: %.cpp
				$(CXX) -c $(CPPFLAGS32_DEBUG) $< -o $@


$(XLIB64_DEBUG): $(XLIB_OBJ64_DEBUG)
				ar rcs -o $@ $^

$(XLIB_OBJ64_DEBUG): %.o64d: %.cpp
				$(CXX) -c $(CPPFLAGS64_DEBUG) $< -o $@


$(XLIB32_RELEASE): $(XLIB_OBJ32_RELEASE)
				ar rcs -o $@ $^

$(XLIB_OBJ32_RELEASE): %.o32r: %.cpp
				$(CXX) -c $(CPPFLAGS32_RELEASE) $< -o $@


$(XLIB64_RELEASE): $(XLIB_OBJ64_RELEASE)
				ar rcs -o $@ $^

$(XLIB_OBJ64_RELEASE): %.o64r: %.cpp
				$(CXX) -c $(CPPFLAGS64_RELEASE) $< -o $@



$(XLIB_TEST32_DEBUG): $(XLIB_TEST_OBJ32_DEBUG) $(XLIB_OBJ32_DEBUG)
				$(CXX) $(CPPFLAGS32_DEBUG) -o $@ $^

$(XLIB_TEST_OBJ32_DEBUG): %.o32d: %.cpp
				$(CXX) -c $(CPPFLAGS32_DEBUG) $< -o $@


$(XLIB_TEST64_DEBUG): $(XLIB_TEST_OBJ64_DEBUG) $(XLIB_OBJ64_DEBUG)
				$(CXX) $(CPPFLAGS64_DEBUG) -o $@ $^

$(XLIB_TEST_OBJ64_DEBUG): %.o64d: %.cpp
				$(CXX) -c $(CPPFLAGS64_DEBUG) $< -o $@


$(XLIB_TEST32_RELEASE): $(XLIB_TEST_OBJ32_RELEASE) $(XLIB_OBJ32_RELEASE)
				$(CXX) $(CPPFLAGS32_RELEASE) -o $@ $^

$(XLIB_TEST_OBJ32_RELEASE): %.o32r: %.cpp
				$(CXX) -c $(CPPFLAGS32_RELEASE) $< -o $@


$(XLIB_TEST64_RELEASE): $(XLIB_TEST_OBJ64_RELEASE) $(XLIB_OBJ64_RELEASE)
				$(CXX) $(CPPFLAGS64_RELEASE) -o $@ $^

$(XLIB_TEST_OBJ64_RELEASE): %.o64r: %.cpp
				$(CXX) -c $(CPPFLAGS64_RELEASE) $< -o $@


clean:
				${RM} $(XLIB32_DEBUG) $(XLIB64_DEBUG)
				${RM} $(XLIB32_RELEASE) $(XLIB64_RELEASE)
				${RM} $(XLIB_OBJ32_DEBUG) $(XLIB_OBJ64_DEBUG)
				${RM} $(XLIB_OBJ32_RELEASE) $(XLIB_OBJ64_RELEASE)
				${RM} $(XLIB_TEST32_DEBUG) $(XLIB_TEST64_DEBUG)
				${RM} $(XLIB_TEST32_RELEASE) $(XLIB_TEST64_RELEASE)
				${RM} $(XLIB_TEST_OBJ32_DEBUG) $(XLIB_TEST_OBJ64_DEBUG)
				${RM} $(XLIB_TEST_OBJ32_RELEASE) $(XLIB_TEST_OBJ64_RELEASE)
