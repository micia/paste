/* See LICENSE file for copyright and license details. */
#include <errno.h>
#include <locale.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <wchar.h>

typedef struct {
	FILE *fp;
	const char *name;
} Fdescr;

static void eprintf(const char *, ...);
static void eusage(void);
static size_t unescape(wchar_t *);
static wint_t in(Fdescr *);
static void out(wchar_t);
static void sequential(Fdescr *, int, const wchar_t *, size_t);
static void parallel(Fdescr *, int, const wchar_t *, size_t);

int
main(int argc, char **argv) {
	const char *adelim = NULL;
	bool seq = false;
	wchar_t *delim;
	size_t len;
	Fdescr *dsc;
	int i, c;
	
	setlocale(LC_CTYPE, "");
	
	while((c = getopt(argc, argv, "sd:")) != -1)
		switch(c) {
		case 's':
			seq = true;
			break;
		case 'd':
			adelim = optarg;
			break;
		case '?':
		default:
			eusage();
			break;
		}
	
	argc -= optind;
	argv += optind;
	if(argc == 0)
		eusage();
	
	/* populate delimiters */
	if(!adelim)
		adelim = "\t";
	
	len = mbstowcs(NULL, adelim, 0);
	if(len == (size_t)-1)
		eprintf("invalid delimiter\n");
	
	delim = malloc((len + 1) * sizeof(*delim));
	if(!delim)
		eprintf("out of memory\n");
	
	mbstowcs(delim, adelim, len);
	len = unescape(delim);
	if(len == 0)
		eprintf("no delimiters specified\n");
	
	/* populate file list */
	dsc = malloc(argc * sizeof(*dsc));
	if(!dsc)
		eprintf("out of memory\n");
	
	for(i = 0; i < argc; i++) {
		const char *name = argv[i];
		
		if(strcmp(name, "-") == 0)
			dsc[i].fp = stdin;
		else
			dsc[i].fp = fopen(name, "r");
		
		if(!dsc[i].fp)
			eprintf("can't open '%s':", name);
		
		dsc[i].name = name;
	}
	
	if(seq)
		sequential(dsc, argc, delim, len);
	else
		parallel(dsc, argc, delim, len);
	
	for(i = 0; i < argc; i++) {
		if(dsc[i].fp != stdin)
			(void)fclose(dsc[i].fp);
	}
	
	free(delim);
	free(dsc);
	return 0;
}

static void
eprintf(const char *fmt, ...) {
	va_list va;
	
	va_start(va, fmt);
	vfprintf(stderr, fmt, va);
	va_end(va);
	if(fmt[0] && fmt[strlen(fmt) - 1] == ':')
		fprintf(stderr, " error %d (%s)\n", errno, strerror(errno));
	
	exit(1);
}

static void
eusage(void) {
	eprintf("usage: paste [-s][-d list] file...\n");
}

static size_t
unescape(wchar_t *delim) {
	wchar_t c;
	size_t i;
	size_t len;
	
	for(i = 0, len = 0; (c = delim[i++]) != '\0'; len++) {
		if(c == '\\') {
			switch(delim[i++]) {
			case 'n':
				delim[len] = '\n';
				break;
			case 't':
				delim[len] = '\t';
				break;
			case '0':
				delim[len] = '\0';
				break;
			case '\\':
				delim[len] = '\\';
				break;
			case '\0':
			default:
				/* POSIX: unspecified results */
				return len;
			}
		} else
			delim[len] = c;
	}
	
	return len;
}

static wint_t
in(Fdescr *f) {
	wint_t c = fgetwc(f->fp);
	
	if(c == WEOF && ferror(f->fp))
		eprintf("'%s' read error:", f->name);
	
	return c;
}

static void
out(wchar_t c) {
	putwchar(c);
	if(ferror(stdout))
		eprintf("write error:");
}

static void
sequential(Fdescr *dsc, int len, const wchar_t *delim, size_t cnt) {
	int i;
	
	for(i = 0; i < len; i++) {
		size_t d = 0;
		wint_t c, last = WEOF;
		
		while((c = in(&dsc[i])) != WEOF) {
			if(last == '\n') {
				if(delim[d] != '\0')
					out(delim[d]);
				
				d++;
				d %= cnt;
			}
			
			if(c != '\n')
				out((wchar_t)c);
			
			last = c;
		}
		
		if(last == '\n')
			out((wchar_t)last);
	}
}

static void
parallel(Fdescr *dsc, int len, const wchar_t *delim, size_t cnt) {
	int last;
	
	do {
		int i;
		
		last = 0;
		for(i = 0; i < len; i++) {
			wint_t c;
			wchar_t d = delim[i % cnt];
			
			do {
				wint_t o = in(&dsc[i]);
				
				c = o;
				switch(c) {
				case WEOF:
					if(last == 0)
						break;
					
					o = '\n';
					/* fallthrough */
				case '\n':
					if(i != len - 1)
						o = d;
					
					break;
				default:
					break;
				}
				
				if(o != WEOF) {
					/* pad with delimiters up to this point */
					while(++last < i) {
						if(d != '\0')
							out(d);
					}
					
					out((wchar_t)o);
				}
			} while(c != '\n' && c != WEOF);
		}
	} while(last > 0);
}
