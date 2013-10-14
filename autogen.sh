#!/bin/sh
#
# autogen.sh glue for CMU Cyrus IMAP
# $Id: autogen.sh,v 1.9 2006/04/18 19:48:36 rollinhand Exp $
#
# Requires: automake, autoconf, dpkg-dev
set -e
test ! ${OSTYPE} && {
	OSTYPE="other"
}

# Check if needed software is available on system
# MSYS is used on Windows
test "${OSTYPE}" = "msys" && {
	echo "Detected Operating System: ${OSTYPE}"
}

# Unix-based operating systems
test ${OSTYPE} != "msys" -a ${OSTYPE} != "cygwin" && {
	echo "Detected Operating System: ${OSTYPE}"
	echo -n "Check Build Environment..."
	for tool in aclocal autoreconf autoheader automake libtoolize intltoolize autoconf; do
		test ! `whereis ${tool} | awk '{print $2}'` && {
			echo " not OK"
			echo "${tool} not found - please install first!"
			exit
		}		
	done
	echo " OK"
}

# Refresh GNU autotools toolchain.
# Test if /usr/share/automake exists, else prove for automake-$version
# This test-case is e.g. needed for SuSE distributions
#automk="automake"
#test ! -d /usr/share/automake && {
#	version=`automake --version | head -n 1 | awk '{print $4}' | awk -F. '{print $1"."$2}'`
#	automk="automake-${version}"
#}

#for i in config.guess config.sub missing install-sh mkinstalldirs ; do
#	test -r /usr/share/${automk}/${i} && {
#		rm -f ${i}
#		cp /usr/share/${automk}/${i} .
#	}
#	chmod 755 ${i}
#done

libtoolize --copy --force
aclocal
autoheader
automake --verbose --copy --add-missing 
glib-gettextize --force --copy
#autoreconf -i -f -v --warnings=all
intltoolize --copy --force
autoconf


# For the Debian build
test -d debian -a "${OSTYPE}" != "msys" -a "${OSTYPE}" != "cygwin" && {
	# Kill executable list first
	`rm -f debian/executable.files`

	# Make sure our executable and removable lists won't be screwed up
	debclean && echo Cleaned buildtree just in case...

	# refresh list of executable scripts, to avoid possible breakage if
	# upstream tarball does not include the file or if it is mispackaged
	# for whatever reason.
	echo Generating list of executable files...
	`rm -f debian/executable.files`
	`find -type f -perm +111 ! -name '.*' -fprint debian/executable.files`

	# link these in Debian builds
#	rm -f config.sub config.guess
#	ln -s /usr/share/misc/config.sub .
#	ln -s /usr/share/misc/config.guess .
}

exit 0
