#pragma once
#include <iostream>
#include <sqlite3.h>
#include <vector>
#include <string>

struct Message {
    std::string sender;
    std::string receiver;
    std::string text;
};

class DatabaseManager {
    private:
        sqlite3* db;
        char* error_message;
    
    public:
        DatabaseManager(){
            int rc = sqlite3_open("server_data.db", &db);
            if(rc != SQLITE_OK){
                std::cerr << "Error: " << error_message << std::endl;
                return;
            }

            std::string users = "CREATE TABLE IF NOT EXISTS users(id INTEGER PRIMARY KEY AUTOINCREMENT, username TEXT UNIQUE , password TEXT);";
            std::string messages = "CREATE TABLE IF NOT EXISTS messages(id INTEGER PRAMARY KEY AUTOINCREMENT, sender, receiver, text, timestamp DATATIME DEFAULT CURRENT_TIMESTAMP);";
            sqlite3_exec(db, users.c_str(), nullptr, nullptr, &error_message);
            sqlite3_exec(db, messages.c_str(), nullptr, nullptr, &error_message); 
        }
        ~DatabaseManager(){
            sqlite3_close(db);
            return ;
        }

        bool register_user(std::string username, std::string password){
            std::string query = "INSERT INTO users VALUES(?, ?);";
            sqlite3_stmt* stmt;
            bool user_exists = true;

            if(sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) == SQLITE_OK ){
                sqlite3_bind_text(stmt, 1 , username.c_str(), -1 ,SQLITE_STATIC);
                sqlite3_bind_text(stmt, 2, password.c_str(), -1, SQLITE_STATIC);

            }
            if(sqlite3_step(stmt) == SQLITE_ROW){
                return true;
            }
            
            sqlite3_finalize(stmt);
            return user_exists;

        }

        bool authenicate_user(std::string username, std::string password){
            sqlite3_stmt* stmt;
            bool user_exists = false;

            std::string query = "SELECT COUNT(*) FROM users WHERE (username = ? AND password = ?);";
            int rc = sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr);
            if(rc == SQLITE_OK){
                return true;
            }
            sqlite3_bind_text(stmt, 1 , username.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 2, password.c_str(), -1, SQLITE_STATIC);

            if(sqlite3_step(stmt) == SQLITE_ROW){
                if(sqlite3_column_int(stmt, 0 ) > 0){
                    return (user_exists = true);
                }
            }
            sqlite3_finalize(stmt);
            return user_exists;
        }

        bool save_messages(std::string sender, std::string receiver, std::string text){
            sqlite3_stmt* stmt;
            std::string query = "INSERT INTO messages(sender, receiver, text) VALUES(?,?,?);";
            if(sqlite3_prepare_v2(db, query.c_str(), -1,&stmt, nullptr) != SQLITE_OK){
                return false;
            }
            sqlite3_bind_text(stmt, 1, sender.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 2, receiver.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 3, text.c_str(), -1, SQLITE_STATIC);

            int rc = sqlite3_step(stmt);
            sqlite3_finalize(stmt);
            return(rc = SQLITE_DONE);
        }

        std::vector<Message> get_chat_history(std::string user1, std::string user2){
            std::vector<Message> history;

            sqlite3_stmt *stmt;

            std::string query = "SELECT sender, receiver, text FROM messages WHERE(sender = ? AND receiver = ?) OR (sender = ? AND receiver = ? ) ORDER BY timestamp ASC;";
            if(sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) == SQLITE_OK){
                sqlite3_bind_text(stmt, 1, user1.c_str(), -1, SQLITE_STATIC);
                sqlite3_bind_text(stmt, 2, user2.c_str(), -1, SQLITE_STATIC);
                sqlite3_bind_text(stmt, 3, user2.c_str(), -1, SQLITE_STATIC);
                sqlite3_bind_text(stmt, 4, user1.c_str(), -1, SQLITE_STATIC);
            }

            while(sqlite3_step(stmt) ==SQLITE_ROW){
                Message msg;
                msg.sender = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
                msg.receiver = reinterpret_cast<const char*>(sqlite3_column_text(stmt,1));
                msg.text = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
                history.emplace_back(msg);
            }
            sqlite3_finalize(stmt);
            return history;
        }
};