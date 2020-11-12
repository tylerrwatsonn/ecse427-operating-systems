This is an implementation of an RPC calculator

The backend and frontend programs require two arguments: host and port
The backend accepts a maximum of 5 connections at a time
Once a "quit" command is executed, the backend waits for the next connection and serves that client
Then, the backend continues running and refuses all other incoming connections until all its children either receive a quit or exit command
Once all children have shutdown, one more incoming connection is required for the backend to terminate

In terms of parsing the frontend input, all non entered arguments default to 0
So "divide 5" will result in a division by 0 error.

Thanks for reading the README!!

