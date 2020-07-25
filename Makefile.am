AUTOMAKE_OPTIONS = std-options
SUBDIRS=src
AM_DISTCHECK_CONFIGURE_FLAGS = --with-systemdsystemunitdir=$$dc_install_base/$(systemdsystemunitdir) 

if HAVE_SYSTEMD
systemdsystemunit_DATA = autolock.service
endif

#if ENABLE_MODPROBE_ABS_PATH
#else
#endif
sbin_SCRIPTS = autolock
sysconf_DATA = autolockd 
EXTRA_DIST = $(systemdsystemunit_DATA) $(sysconf_DATA)

dist-rpm: distdir
	tardir=$(distdir) && $(am__tar) | GZIP=$(GZIP_ENV) gzip -c >$(distdir).tar.gz
	$(am__post_remove_distdir)
	cp -vf $(distdir).tar.gz $(RPMBUILD_DIR)/SOURCES/	
	rpmbuild -ba autolock.spec
	
cppcheck:
	cppcheck --enable=all $(top_srcdir)
	
count:
	sloccount $(top_srcdir)/src
	
comptage:
	cd $(COMPTAGE_ROOT) && perl -w Comptage.pl -r $(abs_top_srcdir) -e "!(.*)\/src(.*)$$"

klocwork_project_name = LDB_$(PACKAGE_NAME)
klocwork_url="http://xbstusclockcd:8070"
klocwork_project_url=$(klocwork_url)/$(klocwork_project_name)
klocwork_project_dir=$(abs_top_srcdir)/klocwork_data
klocwork_project_xmlfile=$(klocwork_project_dir)/KlocWork_$(klocwork_project_name).xml
squore_file=$(top_srcdir)/$(PACKAGE_NAME)-squore.xml

klocwork: 
	kwinject --update make && \
	kwbuildproject -I --url $(klocwork_project_url) --tables-directory $(klocwork_project_dir) kwinject.out && \
	kwadmin --url $(klocwork_url) load $(klocwork_project_name) $(klocwork_project_dir)
#python /applis/klocwork_9.6.3/tools/kwxml/kwxml/kwxml.py  --url $(klocwork_project_url)  --project $(klocwork_project_name)  --user GENAV --xml$(klocwork_project_xmlfile)

klocwork_init:
	mkdir -p $(klocwork_project_dir)
	make clean && kwinject make
	kwbuildproject --url $(klocwork_project_url) --tables-directory $(klocwork_project_dir) kwinject.out
	kwadmin --url $(klocwork_url) load $(klocwork_project_name) $(klocwork_project_dir)

squore:
	/applis/agl_rws_il/bin/squore_batch.sh $(squore_file) -local

	
