# C Chat System Project

## ☁️ Overview

This project implements a multi-user chat system in C using a client-server architecture with peer-to-peer capabilities for direct communication. It features user registration and login, online/offline status tracking, persistent chat history, offline message handling, group chats, and file sharing.

The system consists of two main components:
1.  **Server (`Server.c`)**: Manages user accounts, tracks online users, facilitates initial connections, and stores offline messages. It handles multiple clients concurrently using **Pthreads**.
2.  **Device (`Device.c`)**: The client application that users interact with. It connects to the server for authentication and coordination, and directly to other devices for real-time chat.

Communication relies on the **TCP protocol** for reliability. The system uses text files for data persistence instead of a formal database.

---

## ✨ Features

* **User Authentication**: Secure signup (`REG`) and login (`LOG`) functionality. The server validates usernames and passwords against stored records.
* **Concurrency**: The server uses Pthreads (`pthread_create`, `pthread_exit`) to handle multiple client connections simultaneously.
* **Online Status & Logging**:
    * The server tracks currently online users (`srv/usr_online.txt`) and provides a `list` command to view them.
    * A persistent log (`srv/usr_log.txt`) records usernames, ports, login times, and logout times for all registered users.
* **Offline Messaging**:
    * Users can send messages (`chat`) to offline users. These messages are stored server-side in a dedicated structure (`srv/<recipient>/pendent/<sender>.txt`).
    * Devices can check for users who sent offline messages using the `hanging` command.
    * Devices can retrieve pending messages from a specific user using the `show <username>` command.
* **Online Peer-to-Peer Chat**:
    * When initiating a chat (`chat <username>`) with an online user, the server provides the target user's port.
    * The initiating device then establishes a direct TCP connection with the target device.
    * An invitation mechanism allows the recipient to accept or reject the chat request. If rejected, messages revert to the offline/pending system.
* **Group Chat**:
    * Users can add participants (`\u`) to an ongoing 1-on-1 or group chat.
    * The server provides a list of eligible online users.
    * The new participant establishes direct TCP connections with *all* existing members of the chat.
    * The `select()` system call is used within the `chat()` function to manage multiple P2P socket descriptors efficiently.
* **Chat Features**:
    * **Real-time Messaging**: Messages are sent directly between connected devices. Format: `[dd-MM-yy|hh-mm-ss] username: message`.
    * **Delivery Status**: Sent messages are marked with `*` (offline/left) or `**` (online/received). Received messages have no markers.
    * **File Sharing (`share <filename>`)**: Users can send files directly to all chat participants. The filename and content are transmitted, and receivers save the file locally. Example files (`ex_username.txt`) are created in user directories for testing.
    * **History Deletion (`\d`)**: Clears the local chat history file for the current conversation.
    * **Exiting Chat (`\q`)**: Gracefully leaves the chat, notifying other participants. If a user becomes alone, the chat reverts to offline mode with the server.
* **Persistent Chat History**: Each device stores chat logs locally in text files (`<username>/chat/<peer_username>.txt`).
* **Robust Disconnection Handling**: Uses signal handlers (`signal.h` for SIGINT, SIGTSTP) to manage unexpected disconnections (Ctrl+C, Ctrl+Z), ensuring peers and the server are notified appropriately before exit. Devices also detect and handle server shutdowns during P2P chats.

---

## 🏗️ Architecture & Data Storage

* **Initial Connection**: Devices always connect to the Server first for authentication and to get information about other users.
* **Peer-to-Peer (P2P)**: For online chats, direct TCP connections are established between Devices, reducing server load for message relay. The Server only facilitates the initial handshake.
* **Server as Fallback**: If a user in a chat goes offline, or if a chat is initiated with an offline user, the Server acts as a message store-and-forward agent.
* **Data Storage**: The system uses a simple file-based approach for persistence:
    * **Server (`srv/` directory)**:
        * `usr_all.txt`: List of all registered usernames.
        * `usr_psw.txt`: Stores username-password pairs (plaintext in this implementation).
        * `usr_log.txt`: Login/logout history with timestamps and ports.
        * `usr_online.txt`: List of currently online users with login time and port.
        * `srv/<username>/pendent/<sender_username>.txt`: Stores pending offline messages for `<username>` sent by `<sender_username>`.
    * **Device (`<username>/` directory)**:
        * `chat/<peer_username>.txt`: Local history for the chat with `<peer_username>`.
        * Received shared files are saved directly in the `<username>/` directory.

---

## 📁 File Structure

* `Server.c`: Source code for the chat server.
* `Device.c`: Source code for the chat client (device).
* `makefile`: Instructions for compiling the server and device executables.
* `init.sh`: (Mentioned in report) A script to launch the server and multiple device terminals.
* `srv/`: Directory created by the server to store user data and offline messages.
* `<username>/`: Directory created by each device to store local chat history and received files.

---

## 🚀 Compilation and Execution

### Compilation
1.  Ensure you have `gcc` and `make` installed.
2.  Open a terminal in the project directory.
3.  Run the command:
    ```bash
    make all
    ```
    This will compile `Server.c` (linking with `-lpthread` for concurrency) and `Device.c`, creating executables named `Server` and `Device` respectively.
4.  To clean up compiled files, run:
    ```bash
    make reset
    ```
   

### Execution
1.  **Start the Server**:
    * Open a terminal and run the server, optionally specifying a port (defaults to 4242 if not provided):
        ```bash
        ./Server [port]
        ```
    * The server will create the `srv/` directory if it doesn't exist.
2.  **Start Devices**:
    * The `init.sh` script (as mentioned in the report) is designed to automate launching the server and multiple devices in separate terminals. If you don't have this script, you'll need to do it manually.
    * For *each* device you want to run, open a *new* terminal and execute:
        ```bash
        ./Device <listening_port>
        ```
        Replace `<listening_port>` with a unique port number for this specific device to listen on for incoming P2P connections.
    * The device will first ask for the **Server's port** to connect to.
    * It will then prompt for `login` or `signup`.

---

## 🔧 Usage

### Server Commands
Once the server is running, you can enter commands in its terminal:
* `list`: Shows currently online users with their username, login timestamp, and port.
* `esc`: Gracefully shuts down the server, notifying connected devices.
* `help`: Displays details about the available server commands.

### Device Commands (Main Menu)
After logging in or signing up, the device presents these options:
* `hanging`: Lists users who have sent you messages while you were offline.
* `show <username>`: Displays pending offline messages sent by `<username>` and clears them from the server.
* `chat <username>`: Initiates a chat with `<username>`. Handles both online (P2P) and offline (pending message) scenarios.
* `out`: Logs out the device and closes the connection to the server.

### In-Chat Commands (Device)
While in an active chat session, the following commands are available:
* `<message>`: Type any text and press Enter to send a message to all participants.
* `\u`: Add a user to the current chat (initiates group chat expansion).
* `share <filename>`: Share the specified file (must exist in the device's directory, e.g., `ex_username.txt`) with all participants.
* `\d`: Delete the local chat history file for the current conversation.
* `\q`: Quit the current chat session.

---

## ⚙️ Technical Details

* **Networking**: Uses standard BSD sockets API (`socket`, `bind`, `listen`, `accept`, `connect`, `send`, `recv`) over TCP/IP.
* **I/O Multiplexing**: The `select()` system call is used in the device's chat function to handle input from `stdin`, the server socket, the listener socket, and multiple peer sockets simultaneously.
* **Concurrency**: The server employs a multi-threaded model using Pthreads (`pthread_create`) to manage each connected client in its own thread, improving responsiveness.
* **Signal Handling**: Uses `signal.h` to catch `SIGINT` (Ctrl+C) and `SIGTSTP` (Ctrl+Z) for graceful shutdown procedures.
