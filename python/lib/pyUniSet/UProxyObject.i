/***********************************************************
*           Interface file for wrapping test
*    to regenerate the wrappers run:
*    swig -python UInterface.i
***********************************************************/

%module pyUniSet

%include "std_string.i"

%{
#include "UProxyObject.h"
%}

/* Для генерации классов и констант в Питоне */
%include "UProxyObject.h"
%include "UTypes.h"
%include "UExceptions.h"
