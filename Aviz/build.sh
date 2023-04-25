set -e
export PATH="$PATH:/usr/lib/go-1.10/bin"
#go get ./...
go build
gnome-terminal -- go run *.go
