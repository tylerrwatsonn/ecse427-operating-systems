This is an implementation of a Simple User Level Thread Library

The structure is quite similar to the assignment description. 

There are a total of 4 queues used:

ready_queue (u_context *) - store tasks upon creation and when io request comes back
wait_queue (u_context *) - store tasks while their read io operation is not complete
to_io_queue (char *) - store all io requests (open, read, write, close)
from_io_queue (char *) - store all io responses (from read request)

All io functions (open, read, write, close) are not abstracted.

One single mutex is used to prevent data race when accessing the queues