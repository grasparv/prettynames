all:
	rm -rf testdir
	tar xf testdir.tgz
	go run main.go testdir
