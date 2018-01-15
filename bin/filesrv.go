package main

import (
	"fmt"
	"net"
	"net/http"
	"os"
)

func showURLs(fmts string) {
	ifaces, _ := net.Interfaces()
	for _, iface := range ifaces {
		addrs, _ := iface.Addrs()
		for _, addr := range addrs {
			//var ip net.IP
			switch a := addr.(type) {
			case *net.IPAddr:
				//if ip := a.IP.To4(); ip != nil { fmt.Println("IPAddr:\t", ip, ip.IsLoopback(), iface) }
			case *net.IPNet:
				if ip := a.IP.To4(); ip != nil {
					fmt.Printf(fmts, ip)
					//fmt.Println("IPNet:\t", ip, ip.IsLoopback(), iface)
				}
			}
		}
	}
}

func main() {
	// mux.Handle("/ingamedata/", http.StripPrefix("/ingamedata/", http.FileServer(http.Dir("/tmp/InGameData"))))
	rootdi := http.Dir(".")
	if len(os.Args) > 1 {
		rootdi = http.Dir(os.Args[1])
	}

	handler := http.FileServer(rootdi)
	showURLs("http://%s:8000/\n")
	if err := http.ListenAndServe(":8000", handler); err != nil {
		panic(err)
	}
}
