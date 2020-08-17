# HTTP Client + Server

## Description
This is a simple HTTP client and server. The client is able to `GET` from standard web server (supporting both HTTP/1.0 and HTTP/2.0), and the server is able to handle `GET` requests by sending back HTTP package.

## Usage

### HTTP server
`./http_server PORT`: run the server on the specified PORT

### HTTP client
`./http_client http://hostname[:PORT]/path/to/file`: run the client which sends `GET` request to the given hostname. 

If `:PORT` is not specified, default to port 80. The client is able to handle redirection if the HTTP response status code is "301 Moved Permanently".

## Example

`sudo ./http_server 80`

`./http_server 8888`

`./http_client http://localhost:8888/somedir/somfile.html`

`./http_client http://www.google.com/`
