#LDADD = ../libtopomatch/libtopomatch.a

AM_CFLAGS = @HWLOC_LIBS@
LIBS += @HWLOC_LIBS@


 mapping_CFLAGS = $(AM_CFLAGS) -O3  -Wall -Wextra -pthread
# mapping_CFLAGS = $(AM_CFLAGS) -g  -Wall -Wextra -Qunused-arguments



AM_CPPFLAGS = -I$(srcdir)/../libtopomatch 

bin_PROGRAMS=mapping #kpart #test_ompi

mapping_SOURCES= mapping.c
mapping_LDADD = ../libtopomatch/libtopomatch.la
#kpart_SOURCES= kpart.c
#kpart_LDADD = ../libtopomatch/libtopomatch.la
# test_ompi_SOURCES= test_ompi.c
# test_ompi_LDADD = ../libtopomatch/libtopomatch.la
