.PHONY: all
all: data/tileset.png data/test.fnt

TILEREND = tools/tilerend/tilerend
IMGQUANT = tools/imgquant/imgquant

data/tileset.png: datasrc/rendscript datasrc/dungeon.obj $(TILEREND) $(IMGQUANT)
	$(TILEREND) -o render.png $<
	$(IMGQUANT) render.png -P -o $@ -C 254 -s 16 -os data/tileset.slut

data/test.fnt: /usr/share/fonts/truetype/ubuntu/Ubuntu-R.ttf
	font2glyphmap -size 16 -range 32-126 -padding 0 -o $@ $<

$(TILEREND):
	cd tools/tilerend && $(MAKE)

$(IMGQUANT):
	cd tools/imgquant && $(MAKE)
