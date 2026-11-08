TARGET = WISH30300
TITLE = Wishwood: Kingdom 3030

CC = gcc
CFLAGS = -O3 -Wall -Wextra -Isrc

all: pkg

pkg: $(TARGET).pkg

$(TARGET).pkg: eboot.bin PARAM.SFO sc0/sys/image0.png
	python3 tools/make_gp4.py pkg_root project.gp4
	orbis-pub-cmd img_create --oformat pkg project.gp4 $(TARGET).pkg

eboot.bin: src/main.c src/font.h src/audio.h
	$(CC) $(CFLAGS) src/main.c -o eboot.bin -lm -lpthread -lSceAudioOut -lSceUserService

PARAM.SFO: tools/gen_sfo.py
	python3 tools/gen_sfo.py "$(TITLE)" PARAM.SFO

sc0/sys/image0.png:
	mkdir -p sc0/sys pkg_root/sce_sys
	echo "placeholder" > sc0/sys/image0.png
	cp sc0/sys/image0.png pkg_root/sce_sys/icon0.png || true
	cp PARAM.SFO pkg_root/sce_sys/param.sfo
	cp eboot.bin pkg_root/eboot.bin

clean:
	rm -f eboot.bin PARAM.SFO project.gp4 $(TARGET).pkg
	rm -rf pkg_root sc0
