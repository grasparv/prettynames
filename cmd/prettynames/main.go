package main

import (
	"flag"
	"fmt"

	"github.com/grasparv/prettynames"
)

func main() {
	r := &pretty.Renamer{}
	flag.BoolVar(&r.Dryrun, "n", false, "dry run")
	flag.BoolVar(&r.Hidden, "h", false, "include hidden .files")
	flag.BoolVar(&r.Quiet, "q", false, "be quiet")
	flag.Parse()
	dir := flag.Arg(0)
	if len(dir) == 0 {
		fmt.Printf("no directory given\n")
		return
	}
	err := r.Replace(dir)
	if err != nil {
		fmt.Printf("error %s\n", err.Error())
		return
	}
}
