/***********************************************************
*           Interface file for wrapping test
*    to regenerate the wrappers run:
*    swig -python UInterface.i
***********************************************************/

%module pyUniSet
%{
#include "pyUInterface.h"
%}

/* Для генерации классов и констант в Питоне */
%include "pyUInterface.h"
%include "UTypes.h"
%include "UExceptions.h"
