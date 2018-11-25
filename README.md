# Operating Systems Project
This is a project done during my bachelor degree at the Operating Systems course.

It regards the realization of a service that implements message exchanges. It must accept messages received
by a client and archive them.
The client application allows to read all messages, the possibility to send a new message to any
of the users of the system, which are received in real-time if online, or stored if offline, and the possibility
to cancel some messages received from a user.
The access to the system is done through authentication, using **USERNAME** and **PASSWORD**.
To send a message the any user we have to write it as **RECEIVER|OBJECT|TEXT**.
To communicate with the server we can use 3 commands, all preceded by **#**:
* inbox: allows visualization of messages received
* delete: gives the possibility to cancel the messages of a specific user
* exit: allows the interruption of the application

To start the application run:
```
make
./server
./client
```
To delete the executables run:
```
make clean
```
