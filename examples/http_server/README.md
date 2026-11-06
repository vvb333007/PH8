# HTTP Server

## Building

To build the server, simply run:

```bash
make
```

## Running

Run the generated executable:

```bash
./httpd
```

or 

```bash
httpd.exe
```

The server will start listening on **port 8080**.

Open the following URL in your browser:

```text
http://localhost:8080/
```

The server supports serving static files and executing `.php` scripts from `./www/`.
