.POSIX:

include config.mk

PACKAGE_NAME = sudoku
PACKAGE_DIR = $(PACKAGE_NAME)-$(VERSION)
PACKAGE = $(PACKAGE_DIR).tar.bz2
OBJ_FILES_SOLVER = solver.o
OBJ_FILES_EDITOR = editor.o tui.o term.o util.o

all: $(BIN_NAME_SOLVER) $(BIN_NAME_EDITOR) size

$(BIN_NAME_SOLVER): $(OBJ_FILES_SOLVER)
	$(CC) -o $@ $(OBJ_FILES_SOLVER) $(LDFLAGS)

$(BIN_NAME_EDITOR): $(OBJ_FILES_EDITOR)
	$(CC) -o $@ $(OBJ_FILES_EDITOR) $(LDFLAGS)

$(OBJ_FILES_SOLVER): config.mk
	$(CC) $(CFLAGS_SOLVER) -c $(@:.o=.c)

$(OBJ_FILES_EDITOR): config.mk
	$(CC) $(CFLAGS_EDITOR) -c $(@:.o=.c)

solver.o: solver.c config.h
editor.o: editor.c config.h term.h tui.h util.h
tui.o: tui.c tui.h
term.o: term.c term.h
util.o: util.c util.h

config.h: config.h.in config.mk
solver.6: solver.6.in config.mk
editor.6: editor.6.in config.mk

config.h solver.6 editor.6:
	sed -e "s#%VERSION%#$(VERSION)#g; \
		s#%SOLVER%#$(BIN_NAME_SOLVER)#g; \
		s#%EDITOR%#$(BIN_NAME_EDITOR)#g; \
		s#%WORKDIR%#$(WORKDIR)#g; \
		s#%TITLE_SOLVER%#$(TITLE_SOLVER)#g; \
		s#%TITLE_EDITOR%#$(TITLE_EDITOR)#g" $< > $@

manpages: solver.6 editor.6

size: $(BIN_NAME_SOLVER) $(BIN_NAME_EDITOR)
	size $^

install: all manpages
	@if [ ! -d "$(INSTALL_PATH)" ]; then \
		echo "Creating directory: $(INSTALL_PATH)"; \
		mkdir -p "$(INSTALL_PATH)"; \
	fi
	strip $(BIN_NAME_SOLVER)
	strip $(BIN_NAME_EDITOR)
	cp $(BIN_NAME_SOLVER) "$(INSTALL_PATH)/$(BIN_NAME_SOLVER)"
	cp $(BIN_NAME_EDITOR) "$(INSTALL_PATH)/$(BIN_NAME_EDITOR)"
	chmod 755 "$(INSTALL_PATH)/$(BIN_NAME_SOLVER)"
	chmod 755 "$(INSTALL_PATH)/$(BIN_NAME_EDITOR)"
	@if [ ! -d "$(MANPAGE_PATH)" ]; then \
		echo "Creating directory: $(MANPAGE_PATH)"; \
		mkdir -p "$(MANPAGE_PATH)"; \
	fi
	cp solver.6 "$(MANPAGE_PATH)/$(MAN_NAME_SOLVER)"
	cp editor.6 "$(MANPAGE_PATH)/$(MAN_NAME_EDITOR)"
	chmod 644 "$(MANPAGE_PATH)/$(MAN_NAME_SOLVER)"
	chmod 644 "$(MANPAGE_PATH)/$(MAN_NAME_EDITOR)"

uninstall:
	rm -f "$(INSTALL_PATH)/$(BIN_NAME_SOLVER)"
	rm -f "$(INSTALL_PATH)/$(BIN_NAME_EDITOR)"
	rm -f "$(MANPAGE_PATH)/$(MAN_NAME_SOLVER)"
	rm -f "$(MANPAGE_PATH)/$(MAN_NAME_EDITOR)"

package:
	@if [ -f "$(PACKAGE)" ]; then \
		echo "Removing: $(PACKAGE)" ; \
		rm "$(PACKAGE)"; \
	fi
	@if [ -d "$(PACKAGE_DIR)" ]; then \
		echo "Removing directory: $(PACKAGE_DIR)"; \
		rm -r "$(PACKAGE_DIR)"; \
	fi
	mkdir $(PACKAGE_DIR)
	cp LICENSE Makefile config.mk *.in *.c \
	   tui.h term.h util.h "$(PACKAGE_DIR)/"
	tar -cjf $(PACKAGE) $(PACKAGE_DIR)
	rm -r $(PACKAGE_DIR)

ctags:
	ctags -R --languages=C

clean:
	rm -f $(BIN_NAME_SOLVER) $(BIN_NAME_EDITOR) *.o

distclean: clean
	rm -f config.h solver.6 editor.6 tags

.PHONY: all size install uninstall package ctags manpages clean distclean
