# Tiny-farm
This is a project i developed for the Laboratory 2 course at university. The project consists of implementing multithreading and inter-process communication to optimize computations across multiple files.

# Client
The client uses multithreading to speed up data transmission. To simplify communication, the producer-consumer technique was adopted, where the main thread (_masterworker_) is the producer, and the threads (_workers_) are the consumers.

This system sends one file at a time to each thread, ensuring data insertion when slots are available through the use of `data_items` and `free_slots` semaphores. The `pcmux` mutex ensures that only one thread can access the data at a time, preventing undesirable overwrites by the masterworker or two workers reading the same file from the shared buffer `pcbuff`.

# Masterworker
To check if the server is active, the application attempts to open and close a connection with the server three times at one-second intervals using the `check_server()` function. This prevents the program from closing if the server is active but temporarily unable to receive initialization through the network, helping determine if the server is indeed active.

# Data Transmission
To ensure the correct transmission of data and termination of the server, data is sent by the workers in the following format, with a 4-byte integer:
- If the integer is 0, it indicates the start of data exchange in the following format:
    1. Message length (4 bytes)
    2. Calculated sum (8 bytes)
    3. File name (message length - 8 bytes)
  
  If the received length is negative, it indicates connection closure initiated by the thread.
- Otherwise, it will be interpreted as a server shutdown message.

This system distinguishes data exchange from other actions using the first 4 bytes, while a negative length signals the server threads to terminate or continue exchanging data among threads.

# Server
The server reads the first 4 bytes to determine if it’s a data exchange initiation or a shutdown message. If it's data exchange, the newly created connection is handed over to a thread that will receive data according to the format described above, allowing for faster handling of requests from multiple clients.

# Interrupt Signal Handling
A dedicated thread was chosen to wait for the `SIGINT` signal. This thread shares a variable with the masterworker to signal when `SIGINT` arrives, instructing it to stop sending data to the buffer, terminate the threads, and send the shutdown message to the server.
