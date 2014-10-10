#!/bin/sh -x

for dir in glib20/gettext glib20/gettext/gettext-runtime glib20/gettext/gettext-runtime/libasprintf glib20/gettext/gettext-tools libgd; do
	touch $dir/configure $dir/Makefile.in $dir/aclocal.m4
done

