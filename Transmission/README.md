# Reliable Transmission

A reliable transport protocol using UDP with the properties of retransmission and congestion control. 

## Usage

```
./reliable_sender <rcv_hostname> <rcv_port> <file_path_to_read> <bytes_to_send>
./reliable_receiver <rcv_port> <file_path_to_write>
```

The `<rcv_hostname>` argument is the IP address where the file should be transferred.
The `<rcv_port>` argument is the UDP port that the receiver listens.
The `<file_path_to_read>` argument is the path for the binary file to be transferred.
The `<bytes_to_send>` argument indicates the number of bytes from the specified file to be sent to the receiver.
The `<file_path_to_write>` argument is the file path to store the received data.
