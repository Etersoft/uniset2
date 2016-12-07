/***********************************************************
*           Interface file for wrapping test
*    to regenerate the wrappers run:
*    swig -python UInterface.i
***********************************************************/

%module pyUniSet

%include "std_string.i"

%{
#include "PyUInterface.h"
#include "UProxyObject.h"
%}

/* Для генерации классов и констант в Питоне */
%include "PyUInterface.h"
%include "UTypes.h"
%include "UExceptions.h"
%include "UProxyObject.h"
