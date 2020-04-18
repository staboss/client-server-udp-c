# UDP Client/Server Application
### How to run a server
`./server -a [HOST] -p [PORT]`
- `-h` : show usage
- `-v` : show version
- `-d` : run as daemon
- `-w` : emulate work and sleep N seconds (example: `-w 10`)
- `-l` : specify logfile, default `/tmp/lab2.log` (example: `-l LOGFILE`)

#### Some options may be passed through environment variables:
- `L2ADDR`
- `L2PORT`
- `L2WAIT`
- `L2LOGFILE`

Example: `L2ADDR="127.0.0.1" L2PORT="8080" L2WAIT=10 ./server -d`

### How to use a client
`./client -a [HOST] -p [PORT] -m "MESSAGE"`

Server returns a `"MESSAGE"` in [Base64 encoding](https://en.wikipedia.org/wiki/Base64).

### How to use signals
1. find out the `pid_of_server` with the following command: `lsof -i :[PORT]`
2. use the following command to send a signal: `kill -SIGNAL pid_of_server`
