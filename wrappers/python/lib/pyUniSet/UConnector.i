/***********************************************************
*           Interface file for wrapping test
*    to regenerate the wrappers run:
*    swig -python UInterface.i
***********************************************************/
%include <std_string.i>

%module pyUConnector
%{
#include "UConnector.h"
%}

/* Для генерации классов и констант в Питоне */
%include "UTypes.h"
%include "UConnector.h"
