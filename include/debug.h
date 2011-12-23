#ifndef __DEBUG_H__
#define __DEBUG_H__

#include <stdlib.h>
#include <stdio.h>

#ifdef DEBUG
#define default_debug_level  1
#define debug(level,fmt,args...) ({                          		\
	if ( level <= default_debug_level)                   		\
		printf("%s(%d):"fmt,__FUNCTION__,__LINE__,##args);      \
})                                                           		\

#else
#define debug(level,fmt...)
#endif

#endif
