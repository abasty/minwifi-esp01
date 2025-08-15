/*****************************************************/
/*                                                   */
/*                   TinyFTPClient                   */
/*      (c) Yvan Régeard - All rights reserved       */
/*                                                   */
/* Licensed under the MIT license. See LICENSE file  */
/* in the project root for full license information. */
/*                                                   */
/*****************************************************/
#ifndef _TINY_FTPCLIENT_H_
#define _TINY_FTPCLIENT_H_

#include <ESP8266WiFi.h>
#include <stdint.h>

// Constants
#define FTP_CLIENT_DEFAULT_TIMEOUT 10000 // Default timeout: 10 seconds
#define FTP_CLIENT_BUFFER_SIZE 128       // Internal buffer size: 128 bytes
#define FTP_CLIENT_TRANSFER_BLOCK_SIZE 512 // Block size used during transfer: 512 bytes

// FTPClient class definition
class FTPClient {
    // Private attributes
private:
    // Timeout
    uint16_t m_timeout;

    // WiFi client
    WiFiClient m_client;

    // WiFi passive client
    WiFiClient m_passive_client;

    // Public attribute
public:
    // Last error code
    uint16_t m_last_error_code;

    // Private methods
private:
    // Run command
    uint16_t run_command(const char *command, const char *param = "",
                         char *answer = NULL);

    // Flush available chars
    void flush_available();

    // Get server answer
    uint16_t get_server_answer(char *answer = NULL);

    // Passive mode
    bool open_passive_mode();
    bool close_passive_mode();
    void receive(File &destination_file, ssize_t size);
    void send(File &source_file);

    // Public methods
public:
    // Constructor
    FTPClient(uint16_t timeout = FTP_CLIENT_DEFAULT_TIMEOUT);

    // Connection
    bool open(const char *server_address, const uint16_t server_port,
              const char *user_name, const char *user_password);
    void close();
    bool connected();

    // File management
    bool write_file(const char *file_name, const char *fs_file_name);
    bool read_file(const char *file_name, const char *fs_file_name);
    bool rename_file(const char *from, const char *to);
    bool delete_file(const char *file_name);
    bool get_last_modified_time(const char *file_name, char *result);

    // Directory management
    bool create_directory(const char *directory_name);
    bool remove_directory(const char *directory_name);
    bool change_directory(const char *directory_name);
    ssize_t list_directory(void);
};

#endif
