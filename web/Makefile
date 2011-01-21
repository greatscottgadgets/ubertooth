NANOC := nanoc

compile: 
	$(NANOC) compile

upload: compile
	rsync --delete --exclude .svn -ave ssh output/* ossmann,ubertooth@web.sourceforge.net:htdocs/
