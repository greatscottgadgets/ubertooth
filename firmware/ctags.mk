CTAGS  ?= /usr/bin/ctags-exuberant
CSCOPE ?= /usr/bin/cscope
MKID   ?= /usr/bin/mkid

#
# This requires GNUMake and that /bin/sh is bash.
# NOTES:
#   $${src:0:1} returns the first character in the variable.
#   nullglob: Stop *.h glob from being '*.h' if no headers present.
#

cscope.files:
	@echo "Generating ctags file list for $(TARGET)" 1>&2
	@set -e; \
	for src in $(SRC) $(CPPSRC); \
	do \
		if [ "$${src:0:1}" = "/" ]; \
		then \
			echo $${src}; \
		else \
			echo $(abspath $${src}); \
		fi; \
	done > $@
	@set -e; \
	shopt -s nullglob; \
	for inc in $(CURDIR) $(EXTRAINCDIRS); \
	do \
		for hdr in $${inc}/*.h; \
		do \
			if [ "$${hdr:0:1}" = "/" ]; \
			then \
				echo $${hdr}; \
			else \
				echo $(abspath $${hdr}); \
			fi; \
		done; \
	done >> $@
	@cat $@ | tr '\n' '\0' >$@.0

ctags: cscope.files
	@echo "Generating index files for $(TARGET)" 1>&2
	@${CTAGS} --totals --fields=+iaS --extra="+qf" --c++-kinds="+p" -L $< >tags.log 2>&1
	@rm tags.log
	@${CTAGS} -e --totals --fields=+iaS --extra="+qf" --c++-kinds="-p" -L $< >TAGS.log 2>&1
	@rm TAGS.log
	@$(CSCOPE) -b -q -i $<  >cscope.log 2>&1
	@rm cscope.log
	@$(MKID) --files0-from=$<.0

ctags_clean:
	rm -rf tags TAGS cscope* ID

.PHONY: ctags ctags_clean cscope.files
