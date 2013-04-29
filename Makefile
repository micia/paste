# paste Makefile
# See LICENSE file for copyright and license details.

.POSIX:

##### CONFIGURATION START ######

PREFIX = /usr/local
MANPREFIX = ${PREFIX}

##### CONFIGURATION END ######

VERSION = 0.1
SRC = paste.c
OBJ = ${SRC:.c=.o}
MAN = ${SRC:.c=.1}

all: paste

.c.o:
	${CC} -c ${CFLAGS} $<

paste: ${OBJ}
	${CC} -o $@ ${OBJ} ${LDFLAGS}

clean:
	rm -f paste ${OBJ} paste-${VERSION}.tar.gz

dist: clean
	mkdir -p paste-${VERSION}
	cp -R LICENSE Makefile README paste.1 ${SRC} paste-${VERSION}
	tar -cf paste-${VERSION}.tar paste-${VERSION}
	gzip paste-${VERSION}.tar
	rm -rf paste-${VERSION}

install: all
	mkdir -p "${DESTDIR}${PREFIX}/bin"
	cp -f paste "${DESTDIR}${PREFIX}/bin"
	chmod 755 "${DESTDIR}${PREFIX}/bin/paste"
	mkdir -p "${DESTDIR}${MANPREFIX}/man/man1"
	sed "s/VERSION/${VERSION}/g" < paste.1 > "${DESTDIR}${MANPREFIX}/man/man1/paste.1"
	chmod 644 "${DESTDIR}${MANPREFIX}/man/man1/paste.1"

uninstall:
	rm -f "${DESTDIR}${PREFIX}/bin/paste"
	rmdir "${DESTDIR}${PREFIX}/bin" 1>/dev/null 2>&1 || true
	rm -f "${DESTDIR}${MANPREFIX}/man/man1/paste.1"
	rmdir "${DESTDIR}${MANPREFIX}/man/man1" 1>/dev/null 2>&1 || true
	rmdir "${DESTDIR}${MANPREFIX}/man" 1>/dev/null 2>&1 || true

.PHONY: all clean dist install uninstall
