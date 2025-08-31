CC	?= gcc

BASE_CFLAGS	+=	--std=c11 -Wall -Wpedantic -Wextra -Wvla -Wundef -Wshadow \
			-Wincompatible-pointer-types -pipe
ifneq ($(CC),clang)
BASE_CFLAGS	+= -Wformat-signedness
endif
SAN_CFLAGS	+=	-g -fsanitize=address,undefined
DBG_CFLAGS	+=	-g
OPT_CFLAGS	+=	-O3 -DNDEBUG -flto=auto -fno-omit-frame-pointer -mno-omit-leaf-frame-pointer -g


.PHONY: all
all: san

.PHONY: san
san:
	cmake -DCMAKE_C_COMPILER="$(CC)" -DCMAKE_C_FLAGS="$(BASE_CFLAGS) $(SAN_CFLAGS)" -DCMAKE_COLOR_MAKEFILE="OFF" -DCMAKE_INSTALL_PREFIX="$(abspath ./build_san/install)" -S . -B ./build_san
	cmake --build ./build_san --config Debug -j`nproc 2>/dev/null`
	./build_san/tests.out
	./runtests.sh -e ./build_san/actf

san32:
	cmake -DCMAKE_C_COMPILER="$(CC)" -DCMAKE_C_FLAGS="$(BASE_CFLAGS) $(SAN_CFLAGS) -m32" -DCMAKE_COLOR_MAKEFILE="OFF" -DCMAKE_INSTALL_PREFIX="$(abspath ./build_san32/install)" -DBUILD_TESTS="OFF" -S . -B ./build_san32
	cmake --build ./build_san32 --config Debug -j`nproc 2>/dev/null`
	./build_san32/tests.out
	./runtests.sh -e ./build_san32/actf

.PHONY: dbg
dbg:
	cmake -DCMAKE_C_COMPILER="$(CC)" -DCMAKE_C_FLAGS="$(BASE_CFLAGS) $(DBG_CFLAGS)" -DCMAKE_COLOR_MAKEFILE="OFF" -DCMAKE_INSTALL_PREFIX="$(abspath ./build_dbg/install)" -S . -B ./build_dbg
	cmake --build ./build_dbg --config Debug -j`nproc 2>/dev/null`

.PHONY: opt
opt:
	cmake -DCMAKE_C_COMPILER="$(CC)" -DCMAKE_C_FLAGS="$(BASE_CFLAGS) $(OPT_CFLAGS)" -DCMAKE_COLOR_MAKEFILE="OFF" -DCMAKE_INSTALL_PREFIX="$(abspath ./build_opt/install)" -S . -B ./build_opt
	cmake --build ./build_opt --config Release -j`nproc 2>/dev/null`
	/usr/bin/time -v ./build_opt/actf -q mega-muxer-20221126_ctf2

.PHONY: tags
tags:
	ctags -e --languages=c --langmap=c:+.h --exclude='build*' -R .

.PHONY: clean
clean:
	$(RM) -rf build_san build_opt build_dbg build_san32
