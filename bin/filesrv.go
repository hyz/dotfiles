package main

import "net/http"

func main() {
	fileServer := http.FileServer(http.Dir("./"))
	if err := http.ListenAndServe(":8000", fileServer); err != nil {
		panic(err)
	}
}
