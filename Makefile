CC = clang
CFLAGS = -O3 -Wall -Wextra -isystem $(OO_PS4_TOOLCHAIN)/include -Isrc
LDFLAGS = -L$(OO_PS4_TOOLCHAIN)/lib -lm -lpthread

all: WISH30300.pkg

eboot.bin: src/main.c
	$(CC) $(CFLAGS) src/main.c -o eboot.bin $(LDFLAGS)

PARAM.SFO: tools/gen_sfo.py
	python3 tools/gen_sfo.py

project.gp4: tools/make_gp4.py eboot.bin PARAM.SFO
	python3 tools/make_gp4.py

WISH30300.pkg: project.gp4 eboot.bin PARAM.SFO
	mkdir -p pkg_root/sce_sys
	cp PARAM.SFO pkg_root/sce_sys/param.sfo
	cp eboot.bin pkg_root/eboot.bin
	if [ -f $(OO_PS4_TOOLCHAIN)/bin/create-pkg ]; then \
		$(OO_PS4_TOOLCHAIN)/bin/create-pkg --gp4 project.gp4 --outdir . ; \
	elif [ -f $(OO_PS4_TOOLCHAIN)/bin/pkg-builder ]; then \
		$(OO_PS4_TOOLCHAIN)/bin/pkg-builder --gp4 project.gp4 --outdir . ; \
	else \
		tar -cvf WISH30300.pkg pkg_root/ ; \
	fi

clean:
	rm -f eboot.bin PARAM.SFO project.gp4 WISH30300.pkg
	rm -rf pkg_root sc0
