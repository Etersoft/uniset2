/***********************************************************
*           Interface file for wrapping test
*    to regenerate the wrappers run:
*    swig -python UException.i
***********************************************************/
%include <std_string.i>

%module UExceptions
%{
#include "UExceptions.h"
%}
%include "UExceptions.h"
