#!/bin/sh

exec $1 ${CATCH_TEST_ARGS} -o $1.test.log  1>/dev/null 2>/dev/null
