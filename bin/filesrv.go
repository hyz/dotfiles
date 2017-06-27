package main

import "net/http"
import "os"

func main() {
	di := http.Dir(".")
	if len(os.Args) > 1 {
		di = http.Dir(os.Args[1])
	}
	fileServer := http.FileServer(di)
	println("http://127.0.0.1:8000")
	if err := http.ListenAndServe(":8000", fileServer); err != nil {
		panic(err)
	}
}
