#!/bin/sh

START=uniset-start.sh

${START} -f ./smemory-test --confile test.xml --dlog-add-levels any --unideb-add-levels system --localNode LocalhostNode
