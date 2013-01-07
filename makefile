# Settings
PATH    := $(PATH):../ct
PROG     = pileus.cgi
CC       = gcc
CT       = ct
CFLAGS   = -Wall -Werror -Wno-unused-result -g --std=c99
CPPFLAGS = $(shell pkg-config --cflags glib-2.0) -I../markdown
LDFLAGS  = $(shell pkg-config --libs   glib-2.0) -L../markdown -lmarkdown

# Targets
default: test

all: $(PROG)

test: $(PROG)
	./$(PROG)

clean:
	rm -f src/*.o src/html.c $(PROG)

push: $(PROG)
	SITE="pileus@pileus.org:~/src/site/";              \
	PAGES="pileus@pileus.org:~/src/pages/";            \
	rsync -uavz .htaccess .gitignore $$SITE;           \
	rsync -uavz --delete --exclude '*~' --exclude '.*' \
		pileus.cgi src theme $$SITE;               \
	rsync -uavz --delete --exclude '*~' --exclude '.*' \
		pages/ $$PAGES/;

# Rules
$(PROG): src/main.o src/html.o
	$(CC) $(CFLAGS) -o $@ $+ $(LDFLAGS)

%.o: %.c makefile src/html.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

src/%.c: theme/%.ct makefile
	$(CT) -o $@ $<

.SECONDARY:
