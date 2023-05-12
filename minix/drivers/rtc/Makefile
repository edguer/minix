PROG=rtc
SRCS=rtc.c

FILES=${PROG}.conf
FILESNAME=${PROG}
FILESDIR=/etc/system.conf.d

DPADD+=${LIBCHARDRIVER} ${LIBSYS}
LDADD+=-lchardriver -lsys

.include <minix.service.mk>