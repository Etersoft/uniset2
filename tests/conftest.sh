#!/bin/sh

SID=$1

[ -z "$SID" ] && SID=1

echo "check auto ID configuration..."
uniset-start.sh -f ./conftest --confile test.xml --prop-id2 -10

echo "check id from file configuration..."
uniset-start.sh -f ./conftest --confile testID.xml

