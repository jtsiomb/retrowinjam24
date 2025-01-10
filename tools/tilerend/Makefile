src = $(wildcard src/*.c)
obj = $(src:.c=.o)
dep = $(src:.c=.d)
bin = tilerend

opt = -O3 -ffast-math -fno-strict-aliasing
dbg = -g

CFLAGS = -pedantic -Wall $(dbg) $(opt) -MMD
LDFLAGS = -lmeshfile -limago -lm

$(bin): $(obj)
	$(CC) -o $@ $(obj) $(LDFLAGS)

-include $(dep)

.PHONY: clean
clean:
	rm -f $(obj) $(bin)
