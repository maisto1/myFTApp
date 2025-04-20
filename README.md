# FT File Transfer Project

A client-server system for secure and reliable file transfers.

## Overview

FT File Transfer is a client-server application that enables bidirectional file transfers between clients and servers. The system consists of two main programs:

1. **myFTserver**: A concurrent server that handles multiple read, write, and listing requests from clients
2. **myFTclient**: A client that can upload files to the server, download files from the server, and view listings of files on the server

## Features

- **Bidirectional transfer**: Upload (write) and download (read) files
- **Concurrent handling**: Support for simultaneous connections from multiple clients
- **Remote listing**: View files in remote directories
- **Path management**: Support for custom paths on both client and server sides
- **Error handling**: Robust treatment of exceptions and edge cases
- **Naming flexibility**: Ability to assign different names to local and remote files

## Usage

### Server

```
myFTserver -a <server_address> -p <server_port> -d <ft_root_directory>
```

Parameters:
- `-a <server_address>`: IP address on which the server will listen
- `-p <server_port>`: Port on which the server will listen
- `-d <ft_root_directory>`: Root directory for file storage

### Client

#### Upload (Write a file to the server)

```
myFTclient -w -a <server_address> -p <port> -f <local_path/filename_local> [-o <remote_path/filename_remote>]
```

#### Download (Read a file from the server)

```
myFTclient -r -a <server_address> -p <port> -f <remote_path/filename_remote> [-o <local_path/filename_local>]
```

#### Listing files in a remote directory

```
myFTclient -l -a <server_address> -p <port> -f <remote_path/>
```

Common parameters:
- `-a <server_address>`: Server IP address
- `-p <port>`: Server port
- `-f <path/filename>`: Path and filename (local or remote)
- `-o <path/filename>`: Destination path and filename (optional)

## Error Handling

The system handles various error conditions, including:
- Incorrect or missing invocation parameters
- Non-existent files during read operations
- Insufficient storage space
- IP and port binding errors
- Connection interruptions
- Concurrent access to the same file

## Implementation Notes

- The server automatically creates the specified root directory if it doesn't exist
- Concurrent write requests to the same file are safely handled
- Listing operations generate output similar to the Unix `ls -la` command

## System Requirements

- Operating system compatible with TCP/IP sockets
- Sufficient permissions to create, read and write files in the specified directories
- Available network ports not blocked by firewalls

## Usage Example

1. Starting the server:
   ```
   myFTserver -a 127.0.0.1 -p 8080 -d /home/user/ftserver
   ```

2. Uploading a file:
   ```
   myFTclient -w -a 127.0.0.1 -p 8080 -f document.txt -o backup/doc.txt
   ```

3. Viewing files in the remote directory:
   ```
   myFTclient -l -a 127.0.0.1 -p 8080 -f backup/
   ```

4. Downloading a file:
   ```
   myFTclient -r -a 127.0.0.1 -p 8080 -f backup/doc.txt -o retrieved_document.txt
   ```
