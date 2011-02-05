NANOC := nanoc

compile: 
	$(NANOC) compile

upload: compile
	rsync --delete --exclude .svn -ave ssh -i ~/.ssh/id_rsa output/* ossmann,ubertooth@web.sourceforge.net:htdocs/
