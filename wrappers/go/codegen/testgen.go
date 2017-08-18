package main

import (
	"uniset"
	"fmt"
)

type TestGen struct {
	TestGen_SK
}

func main() {

	uniset.Init("test.xml")

	tobj := TestGen{}

	Init_TestGen(&tobj,"TestProc1","TestProc")

	fmt.Printf("TestObject: %s\n",tobj.myname)
	fmt.Printf("TestObject input1_s: %d\n",tobj.input1_s)
	fmt.Printf("TestObject id: %d\n",tobj.id)
	fmt.Printf("TestObject test_int: %d\n",tobj.test_int)
}

