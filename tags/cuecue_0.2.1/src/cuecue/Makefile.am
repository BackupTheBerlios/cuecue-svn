

bin_PROGRAMS = cuecue

if WIN32
CLEANFILES = cuecue.res

cuecue_LDADD = ../libcuecue/libcuecue.a cuecue.res
cuecue.res: cuecue.rc
	windres.exe -i cuecue.rc -I rc -o cuecue.res -O coff
cuecue_SOURCES = cuecue.c cuecue.rc
else
cuecue_LDADD = ../libcuecue/libcuecue.a
cuecue_SOURCES = cuecue.c
endif


