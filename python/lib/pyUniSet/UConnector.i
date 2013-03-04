/***********************************************************
*           Interface file for wrapping test
*    to regenerate the wrappers run:
*    swig -python UInterface.i
***********************************************************/

%module pyUConnector
%{
#include "UConnector.h"
%}

/* Для генерации классов и констант в Питоне */
%include "UTypes.h"
//%include "UExceptions.h"
%include "UConnector.h"
