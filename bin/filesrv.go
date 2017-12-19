package main

import "net/http"
import "os"

func main() {
	// mux.Handle("/ingamedata/", http.StripPrefix("/ingamedata/", http.FileServer(http.Dir("/tmp/InGameData"))))
	rootdi := http.Dir(".")
	if len(os.Args) > 1 {
		rootdi = http.Dir(os.Args[1])
	}

	handler := http.FileServer(rootdi)
	println("http://127.0.0.1:8000")
	if err := http.ListenAndServe(":8000", handler); err != nil {
		panic(err)
	}
}
