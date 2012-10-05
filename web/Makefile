NANOC := nanoc

compile: 
	$(NANOC) compile

# Add the following to your .ssh/config file to use upload
#Host web.sourceforge.net
#     User <SF username>,ubertooth
upload: compile
	rsync --delete --exclude .svn -ave ssh output/* web.sourceforge.net:htdocs/

clean:
	rm -rf output tmp
