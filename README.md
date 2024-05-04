# Port Scanner

This is a simple port scanner written in C++. It can scan ports of a hostname or IP address.

## Usage

```sh
./portscanner -host=<hostname/IP>[,<hostname/IP>,...] [-port=<port>]
```

- `<hostname/IP>[,<hostname/IP>,...]`: Specify one or more hostnames or IP addresses to scan, separated by commas. Multiple hosts trigger a port sweep.
- `<port>` (optional): Specify the port to scan. If not provided, all ports (1-65534) will be scanned.

Example:
```sh
./portscanner -host=example.com
./portscanner -host=192.168.1.1 -port=80
./portscanner -host=example.com,192.168.1.1,2.2.2.*
```

## Features

- Supports scanning of multiple hosts/IPs.
- Uses non-blocking socket for faster scanning.

## Notes

- This tool is for educational purposes only. Use it responsibly and only on systems you have permission to scan.

