// Command copydocs refreshes the embedded copy of a docs file that lives
// outside the fxmcp module tree. go:embed cannot reference paths above the
// embedding file's directory (e.g. ../../docs/...), so
// docs/COMMAND_LINE_OPTIONS.md is mirrored into
// internal/mcpserver/docsdata for embedding. Run `go generate ./...` from
// the fxmcp module root after that canonical file changes.
package main

import (
	"flag"
	"io"
	"log"
	"os"
)

func main() {
	src := flag.String("src", "", "source file to copy")
	dst := flag.String("dst", "", "destination file")
	flag.Parse()

	if *src == "" || *dst == "" {
		log.Fatal("both -src and -dst are required")
	}

	in, err := os.Open(*src)
	if err != nil {
		log.Fatalf("open %s: %v", *src, err)
	}
	defer in.Close()

	out, err := os.Create(*dst)
	if err != nil {
		log.Fatalf("create %s: %v", *dst, err)
	}
	defer out.Close()

	if _, err := io.Copy(out, in); err != nil {
		log.Fatalf("copy: %v", err)
	}
}
