#include <iostream>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <vector>
#include <string>
#include <cstring>

using boost::asio::ip::tcp;

void show_chat_menu(boost::asio::ssl::stream<boost::asio::ip::tcp::socket>& ssl_socket) {
    while(true) {
        std::cout << "Chat Menu \n" << std::endl;
        std::cout << "1) Write message / View history" << std::endl;
        std::cout << "2) Log out" << std::endl;
        std::cout << "Choice: ";
        std::string choice;
        std::cin >> choice;

        if(choice == "1") {
            std::cout << "Enter companion nickname: ";
            std::string companion;
            std::cin >> companion;

            std::string history_packet = "4" + companion;
            boost::asio::write(ssl_socket, boost::asio::buffer(history_packet));

            char reply[2048];
            std::memset(reply, 0, sizeof(reply));
            size_t len = ssl_socket.read_some(boost::asio::buffer(reply));
            std::string server_reply(reply, len);
            
            std::cout << "\nChat history with " << companion << std::endl;
            std::cout << server_reply << std::endl;

            std::cout << "Enter your message (or press enter to skip): ";
            std::cin.ignore();
            std::string message_text;
            std::getline(std::cin, message_text);

            if(!message_text.empty()) {
               std::string msg_packet = "3" + companion + "|" + message_text;
               boost::asio::write(ssl_socket, boost::asio::buffer(msg_packet));

               std::memset(reply, 0, sizeof(reply));
               ssl_socket.read_some(boost::asio::buffer(reply));
               std::cout << "Message sent!" << std::endl;
            }
        } 
        else if (choice == "2") {
            std::cout << "Logging out" << std::endl;
            break; 
        }
    }
}

void show_auth_menu(boost::asio::ssl::stream<boost::asio::ip::tcp::socket>& ssl_socket) {
    while(true) {
        std::cout << "\n=== Messenger ===" << std::endl;
        std::cout << "1) Registration" << std::endl; 
        std::cout << "2) Login" << std::endl;      
        std::cout << "Choice: ";
        std::string choice;
        std::cin >> choice;
        
        if(choice == "1" || choice == "2") {
            std::string username, password;
            std::cout << "Enter username: ";
            std::cin >> username;
            
            std::cout << "Enter password: ";
            std::cin >> password;

            std::string auth_packet = choice + username + "|" + password;
            boost::asio::write(ssl_socket, boost::asio::buffer(auth_packet));

            char reply[1024];
            std::memset(reply, 0, sizeof(reply));
            size_t len = ssl_socket.read_some(boost::asio::buffer(reply));
            std::string response(reply, len);

            if(response == "1") {
                std::cout << "Success!" << std::endl;
                if(choice == "2") {
                    show_chat_menu(ssl_socket); 
                }
            } else {
                std::cout << "Error! Access denied or username taken." << std::endl;
            }
        }
    }
}

int main() {
    try {
        boost::asio::io_context io;
        boost::asio::ssl::context ssl_context(boost::asio::ssl::context_base::tls_client);
        
        ssl_context.set_options(
            boost::asio::ssl::context::default_workarounds
            | boost::asio::ssl::context::no_sslv2
            | boost::asio::ssl::context::no_sslv3
        );
        ssl_context.set_verify_mode(boost::asio::ssl::verify_none);

        boost::asio::ssl::stream<tcp::socket> ssl_socket(io, ssl_context);
        boost::asio::ip::tcp::resolver resolver(io);
        auto endpoints = resolver.resolve("127.0.0.1", "8080");

        std::cout << "Connecting to server" << std::endl;
        boost::asio::connect(ssl_socket.lowest_layer(), endpoints);

        std::cout << "Performing SSL Handshake..." << std::endl;
        ssl_socket.handshake(boost::asio::ssl::stream_base::client);
        std::cout << "Secure connection established!" << std::endl;

        show_auth_menu(ssl_socket);

    } catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }
    return 0;
}