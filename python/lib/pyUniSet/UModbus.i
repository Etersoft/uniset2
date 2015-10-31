/***********************************************************
*           Interface file for wrapping test
*    to regenerate the wrappers run:
*    swig -python UInterface.i
***********************************************************/
%include <std_string.i>

%module pyUModbus
%{
#include "UModbus.h"
%}

/* Для генерации классов и констант в Питоне */
%include "UModbus.h"
%include "UTypes.h"
%include "UExceptions.h"
