package main

import (
	"flag"
	"fmt"

	"github.com/grasparv/prettynames/pretty"
)

func main() {
	r := &pretty.Renamer{}
	flag.BoolVar(&r.Dryrun, "n", false, "dry run")
	flag.BoolVar(&r.Hidden, "d", false, "include hidden .dot files")
	flag.BoolVar(&r.Quiet, "q", false, "be quiet")
	flag.Parse()
	dirs := flag.Args()
	if len(dirs) == 0 {
		fmt.Printf("no directory given\n")
		return
	}
	for _, dir := range dirs {
		err := r.Replace(dir)
		if err != nil {
			fmt.Printf("error %s\n", err.Error())
			return
		}
	}
}
