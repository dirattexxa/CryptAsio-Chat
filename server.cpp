#include <iostream>
#include <string>
#include <vector>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <utility>
#include "DatabaseManager.cpp"

using boost::asio::ip::tcp;

class Session : public std::enable_shared_from_this<Session> {
    private:
        boost::asio::ssl::stream<tcp::socket> ssl_socket;
        char buff[1024];
        DatabaseManager& db;
        std::string username_session;

    public:
        Session(boost::asio::ip::tcp::socket socket, boost::asio::ssl::context &ssl_context, DatabaseManager& database)
        :   ssl_socket(std::move(socket), ssl_context),
            db(database)
        {

        }

        void start(){
            auto self(shared_from_this());
            ssl_socket.async_handshake(boost::asio::ssl::stream_base::server, [self, this](const boost::system::error_code& handshake_error){
                if(!handshake_error){
                    std::cout << "Safe channel is successfully created" << std::endl;
                    do_read();
                }
            });
        }

    private:
        void do_read(){
            std::memset(buff, 0 , sizeof(buff));
            struct msg {
                std::string sender;
                std::string text;
            };
            auto self(shared_from_this());
            ssl_socket.async_read_some(boost::asio::buffer(buff, 1024),[this, self](const boost::system::error_code& read_error, std::size_t bytes_transferred){
                if(!read_error){
                    std::string data(buff, bytes_transferred);

                    if(data.empty()){
                        do_read();
                        return;
                    }
                    
                    char action_type = data[0];
                    std::string payload = data.substr(1);

                    size_t separator = payload.find('|');
                    
                    if(action_type == '1'){
                        if(separator != std::string::npos){
                            std::string username = payload.substr(0, separator);
                            std::string password = payload.substr(separator + 1);

                            bool success = db.register_user(username, password);
                            std::string response = success ? "1" : "0";

                            boost::asio::async_write(ssl_socket, boost::asio::buffer(response), 
                            [self](const boost::system::error_code&, std::size_t){});
                        }
                    }

                    if(action_type == '2'){
                        if(separator != std::string::npos){
                            std::string username = payload.substr(0, separator);
                            std::string password = payload.substr(separator + 1);

                            bool success = db.authenicate_user(username, password);
                            if(success){
                                self->username_session = username;
                            }
                            std::string response = success ? "1" : "0";

                            boost::asio::async_write(ssl_socket, boost::asio::buffer(response), [self](const boost::system::error_code& write_error, std::size_t bytes){});
                        }
                    }

                    else if(action_type == '3'){
                        if(separator != std::string::npos){
                            std::string receiver = payload.substr(0, separator);
                            std::string text = payload.substr(separator + 1);

                            self->db.save_messages(self->username_session, receiver, text);
                            std::cout << "Message from " << self->username_session << " to " << receiver << "saived" << std::endl;

                            std::string response = "1";
                            boost::asio::async_write(self->ssl_socket, boost::asio::buffer(response), [self, response](const boost::system::error_code& write_error, std::size_t){
                                if(!write_error){
                                    self->do_read();
                                }
                            });
                        }
                    }

                     else if(action_type == '4'){ 
                        std::string chat_with = payload;

                        auto history = db.get_chat_history(username_session, chat_with); 
                        std::string response_history = "";
                        for(const auto& msg : history){
                            response_history += msg.sender + ": " + msg.text + "\n";
                        }
                        if(response_history.empty()){
                            response_history = "Chat history is empty!\n";
                        }

                        boost::asio::async_write(ssl_socket, boost::asio::buffer(response_history),
                            [self, response_history](const boost::system::error_code&, std::size_t){});
                    }
                }else{
                    std::cout << "Client disconnected" << std::endl;
                }
            });
        }

};

class Server {
    private:
        boost::asio::ip::tcp::acceptor acceptor;
        boost::asio::ssl::context ssl_context;
        DatabaseManager& db;
        
    public:
        Server(boost::asio::io_context& io_context, short port, boost::asio::ssl::context& ssl_context ,DatabaseManager& database)
        :   acceptor(io_context, tcp::endpoint(tcp::v4(), port)),
            ssl_context(std::move(ssl_context)),
            db(database)
        {
            do_accept();
        }

    private:
        void do_accept(){
            acceptor.async_accept( [this](const boost::system::error_code& accept_error, tcp::socket socket){
                if(!accept_error){
                    std::make_shared<Session>(std::move(socket), ssl_context, db)->start();
                }
                do_accept();
            });  
        }

};

int main(){
    try{
        boost::asio::io_context io;
        DatabaseManager db;

        boost::asio::ssl::context ssl_context(boost::asio::ssl::context::tls_server);
        ssl_context.set_options(
            boost::asio::ssl::context::default_workarounds
            |   boost::asio::ssl::context::no_sslv2
            |   boost::asio::ssl::context::no_sslv3
            |   boost::asio::ssl::context::single_dh_use
        );

        ssl_context.use_certificate_chain_file("server.crt");
        ssl_context.use_private_key_file("server.key", boost::asio::ssl::context::pem);

        Server server(io, 8080, ssl_context, db);

        io.run();
    }catch(std::exception& e){
        std::cout << e.what() << std::endl;
    }
    return 0;
}