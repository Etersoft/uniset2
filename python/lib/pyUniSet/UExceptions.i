/***********************************************************
*           Interface file for wrapping test
*    to regenerate the wrappers run:
*    swig -python UException.i
***********************************************************/
%include <std_string.i>

%module pyUExceptions
%{
#include "UExceptions.h"
%}
%include "UExceptions.h"
