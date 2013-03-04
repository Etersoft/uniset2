/***********************************************************
*           Interface file for wrapping test
*    to regenerate the wrappers run:
*    swig -python UInterface.i
***********************************************************/

%module pyUniSet
%{
#include "UInterface.h"
%}

/* Для генерации классов и констант в Питоне */
%include "UInterface.h"
%include "UTypes.h"
%include "UExceptions.h"
