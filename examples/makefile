# Configure pkg-config with the path to an installed actf to be able
# to build.
#
# For example if you have run `make dbg && make -C build_dbg install`
# in the root of actf, you can then run the following to build the
# examples: `PKG_CONFIG_PATH=../build_dbg/install/lib/pkgconfig make`

all: count philo

count: count.c
	$(CC) -o $@ $^ `pkg-config --static --cflags --libs actf`

philo: philo.c
	$(CC) -o $@ $^ `pkg-config --static --cflags --libs actf`

.PHONY: clean
clean:
	$(RM) count philo
