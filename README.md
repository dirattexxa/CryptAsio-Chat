# CryptAsio-Chat

A secure, asynchronous console messenger built on a client-server architecture using C++20.

## 🛠 Features
- **Asynchronous Architecture:** Powered by `Boost.Asio` to handle multiple concurrent client connections seamlessly.
- **End-to-End Security:** Integrated `OpenSSL` to establish TLS encrypted communication channels.
- **Database Management:** Server-side `SQLite` integration for secure user authentication, registration, and persistent chat history.
- **Custom Protocol:** Light-weight network protocol for efficient packet parsing.

## 💻 Tech Stack
- **Language:** C++20
- **Networking:** Boost.Asio
- **Security/Crypto:** OpenSSL (TLS)
- **Database:** SQLite3
- **OS Target:** Linux (Arch/Debian)
