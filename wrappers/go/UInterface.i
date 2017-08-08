%module uniset_internal_api

%include "std_string.i"

%{
#include "goUInterface.h"
#include "UProxy.h"
#include "UTypes.h"
%}

//%insert(go_wrapper) %{

//#cgo LDFLAGS: -lgoUniSet

//%}

/* Для генерации классов и констант */
%include "goUInterface.h"
%include "UTypes.h"
%include "UExceptions.h"
%include "UProxy.h"
