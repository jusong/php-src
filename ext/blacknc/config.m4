PHP_ARG_ENABLE(blacknc,
		[ Whether to enable blacknc extension],
		[ enable-blacknc	Enable blacknc extension])

if test $PHP_BLACKNC != "no"; then
	PHP_SUBST(BLACKNC_SHARED_LIBADD)
	PHP_NEW_EXTENSION(blacknc, blacknc.c, $ext_shared)
fi
