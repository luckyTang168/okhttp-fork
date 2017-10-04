//
// Created by Good_Pudge.
//

#include "../include/SSLSocket.hpp"
#include "../include/Exception.hpp"
#include "../util/util.hpp"
#include <openssl/ssl.h>
#include <cstring>

namespace ohf {
    struct OpenSSLInit {
        OpenSSLInit() {
            SSL_library_init();
            SSLeay_add_ssl_algorithms();
            SSL_load_error_strings();
        }
    };

    OpenSSLInit globalInitSSL;

    struct SSLSocket::impl {
        SSL *ssl;
        SSL_CTX *ssl_context;
    };

    SSLSocket::SSLSocket(const Protocol &protocol) : Socket(), pImpl(new impl) {
        const SSL_METHOD *method;
        switch (protocol) {
            case Protocol::SSLv23:
                method = SSLv23_method();
                break;
            case Protocol::SSLv3:
                method = SSLv3_method();
                break;
            case Protocol::TLSv1:
                method = TLSv1_method();
                break;
            case Protocol::TLSv1_1:
                method = TLSv1_1_method();
                break;
            case Protocol::TLSv1_2:
                method = TLSv1_2_method();
                break;
        }

        pImpl->ssl_context = SSL_CTX_new(method);
        if (!pImpl->ssl_context)
            throw Exception(Exception::Code::SSL_CONTEXT_CREATE_ERROR, "SSL context create error:" +
                                                                       util::getOpenSSLError());

        pImpl->ssl = SSL_new(pImpl->ssl_context);
        if (!pImpl->ssl)
            throw Exception(Exception::Code::SSL_CREATE_ERROR, "SSL create error: " +
                                                               util::getOpenSSLError());
    }

    SSLSocket::~SSLSocket() {
        SSL_CTX_free(pImpl->ssl_context);
        SSL_free(pImpl->ssl);
        delete pImpl;
    }

    void SSLSocket::sni(const std::string &name) {
        SSL_set_tlsext_host_name(pImpl->ssl, name.c_str());
    }

    std::iostream &SSLSocket::connect(const std::string &address, const int &port) {
        std::iostream &ios = Socket::connect(address, port);

        SSL_set_fd(pImpl->ssl, socket_fd);
        if (SSL_connect(pImpl->ssl) < 1)
            throw Exception(Exception::Code::SSL_CONNECTION_CREATE_ERROR,
                            "SSL connection create error: " + util::getOpenSSLError());

        return ios;
    }

    void SSLSocket::send(const char *data, int size) {
        int len = SSL_write(pImpl->ssl, data, size);
        if (len < 0) {
            int error = SSL_get_error(pImpl->ssl, len);
            if (error == SSL_ERROR_WANT_WRITE || error == SSL_ERROR_WANT_READ)
                return;
            throw Exception(Exception::Code::SSL_ERROR, "SSL error: " + util::getOpenSSLError());
        }
    }

    std::vector<char> SSLSocket::receive(size_t size) {
        std::vector<char> buffer(size);
        int len = SSL_read(pImpl->ssl, &buffer.at(0), size);
        if (len < 0) {
            int error = SSL_get_error(pImpl->ssl, len);
            if (error == SSL_ERROR_WANT_WRITE || error == SSL_ERROR_WANT_READ)
                return buffer;
            throw Exception(Exception::Code::SSL_ERROR, "SSL error: " + util::getOpenSSLError());
        }
        return std::vector<char>(buffer.begin(), buffer.begin() + len);
    }
}
