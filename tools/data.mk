.PHONY: all
all: data/test.fnt

data/test.fnt: /usr/share/fonts/truetype/ubuntu/Ubuntu-R.ttf
	font2glyphmap -size 16 -range 32-126 -padding 0 -o $@ $<
