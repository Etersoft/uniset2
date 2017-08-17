package main

import (
	"uniset_internal_api"
	"fmt"
)

type TestGen struct {
	TestGen_SK
}

func main() {

	params := uniset_internal_api.ParamsInst()
	params.Add_str("--confile")
	params.Add_str("test.xml")

	err := uniset_internal_api.Uniset_init_params(params, "test.xml")
	if !err.GetOk() {
		panic(err.GetErr())
	}

	tobj := TestGen{}

	Init_TestGen(&tobj,"TestProc1","TestProc")

	fmt.Printf("TestObject: %s\n",tobj.myname)
	fmt.Printf("TestObject input1_s: %d\n",tobj.input1_s)
	fmt.Printf("TestObject id: %d\n",tobj.id)
}

