// stdlib
#include <iostream>
#include <cstring>
#include <iomanip>

// crypto
// #include <jwt-cpp/jwt.h>
#include <openssl/sha.h>

// db
#include <soci/soci.h>
#include <soci/sqlite3/soci-sqlite3.h>

// web
#include "crow_all.h"


crow::App<crow::CookieParser> app;

soci::session db (soci::sqlite3, "../userdb");



bool verify_pass(const std::string& username,const std::string& password) {
    unsigned char hashed_username[SHA256_DIGEST_LENGTH], hashed_password[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char  *>(username.c_str()), username.size(), hashed_username);
    SHA256(reinterpret_cast<const unsigned char  *>(password.c_str()), password.size(), hashed_password);

    std::stringstream username_stream, password_stream;

    for(int i{0}; i < SHA256_DIGEST_LENGTH; i++) {
        username_stream << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hashed_username[i]);
        password_stream << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hashed_password[i]);
    }

    std::string pass;
    db << "select password from accounts where username = '" << username_stream.str() << "'", soci::into(pass);

    if(!pass.empty() && pass == password_stream.str()) {
        return true;
    } else {
        return false;
    }
}

int main()
{

    CROW_ROUTE(app, "/login").methods(crow::HTTPMethod::POST)
    (
        [](const crow::request &req, crow::response& res)
        {
            std::string auth = req.get_header_value("Authorization");
            std::string credentials = auth.substr(6);
            credentials = crow::utility::base64decode(credentials, credentials.size());
            size_t colon = credentials.find(':');
            std::string username = credentials.substr(0, colon), password = credentials.substr(colon+1);
            res.code = verify_pass(username, password) ? 200 : 401;
            res.end();
        });

    app.port(18080).run();
}