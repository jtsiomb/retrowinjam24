.PHONY: all
all: data/tileset.png data/test.fnt

data/tileset.png: datasrc/rendscript datasrc/dungeon.obj tools/tilerend/tilerend
	tools/tilerend/tilerend -o render.png $<
	imgquant render.png -P -o $@ -C 254 -s 16 -os data/tileset.slut

data/test.fnt: /usr/share/fonts/truetype/ubuntu/Ubuntu-R.ttf
	font2glyphmap -size 16 -range 32-126 -padding 0 -o $@ $<
