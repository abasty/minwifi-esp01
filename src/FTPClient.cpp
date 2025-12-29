/*****************************************************/
/*                                                   */
/*                   TinyFTPClient                   */
/*      (c) Yvan Régeard - All rights reserved       */
/*                                                   */
/* Licensed under the MIT license. See LICENSE file  */
/* in the project root for full license information. */
/*                                                   */
/*****************************************************/

#include <LittleFS.h>

#include "FTPClient.h"
#include "os.h"

// FTPClient public functions

// Constructor
FTPClient::FTPClient(uint16_t timeout)
    : m_timeout(timeout), m_last_error_code(0) {}

// Open connection
bool FTPClient::open(const char *server_address, const uint16_t server_port,
                     const char *user_name, const char *user_password) {
    // Connect to server
    if (m_client.connect(server_address, server_port)) {
        // Check if server is ready for a new user
        char banner[FTP_CLIENT_BUFFER_SIZE];
        if (get_server_answer(banner) == 220) {
            // Flush banner
            flush_available();
            hal_print_string(banner);
            // Run USER command
            if (run_command("USER ", user_name) == 331) {
                // Run PASS command
                return (run_command("PASS ", user_password) == 230);
            }
        }
    }

    // Return: error
    return false;
}

// Close connection
void FTPClient::close() {
    // Run QUIT command
    run_command("QUIT");

    // Close WiFi client
    m_client.stop();
}

bool FTPClient::connected() { return m_client.connected(); }

// Write data to file
bool FTPClient::write_file(const char *file_name,
                           const char *fs_file_name) {

    #ifdef ESP32
    char rname[FILE_NAME_SIZE + 4] = "/";
    strncat(rname, fs_file_name, FILE_NAME_SIZE + 3);
    fs_file_name = rname;
    #endif

    // Open source file
    File source_file = LittleFS.open(fs_file_name, "r");
    if (source_file) {
        run_command("TYPE I");
        // Open passive mode
        if (open_passive_mode()) {
            // Run STOR command
            if (run_command("STOR ", file_name) == 150) {
                // Send file
                send(source_file);
            }

            // Close passive mode
            return close_passive_mode();
        }
    }
    // }

    // Return: error
    return false;
}

// Read file
bool FTPClient::read_file(const char *file_name, const char *fs_file_name) {
    // Get file size
    char size_str[FTP_CLIENT_BUFFER_SIZE];
    if (run_command("SIZE ", file_name, size_str) != 213)
        return false;

    ssize_t size = strtol(size_str, 0, 10);
    if (size <= 0)
        return false;

    #ifdef ESP32
    char rname[FILE_NAME_SIZE + 4] = "/";
    strncat(rname, fs_file_name, FILE_NAME_SIZE + 3);
    fs_file_name = rname;
    #endif

    // Open destination file
    File destination_file = LittleFS.open(fs_file_name, "w");
    if (destination_file) {
        run_command("TYPE I");
        // Open passive mode
        if (open_passive_mode()) {
            // Run RETR command
            if (run_command("RETR ", file_name) == 150) {
                // Receive file
                receive(destination_file, size);
            }

            // Close passive mode
            // return
            close_passive_mode();

            return true;
        }
    }

    // Return: error
    return false;
}

// Rename file
bool FTPClient::rename_file(const char *from, const char *to) {
    // Run RNFR command
    if (run_command("RNFR ", from) == 350) {
        // Run RNTO command
        return (run_command("RNTO ", to) == 250);
    }

    // Return: error
    return false;
}

// Delete file
bool FTPClient::delete_file(const char *file_name) {
    // Run DELE command
    return (run_command("DELE ", file_name) == 250);
}

// Get last modified time
bool FTPClient::get_last_modified_time(const char *file_name, char *result) {
    // Run MDTM command
    return (run_command("MDTM ", file_name, result) == 213);
}

// Create directory
bool FTPClient::create_directory(const char *directory_name) {
    // Run MKD command
    return (run_command("MKD ", directory_name) == 257);
}

// Remove directory
bool FTPClient::remove_directory(const char *directory_name) {
    // Run RMD command
    return (run_command("RMD ", directory_name) == 250);
}

// Change directory
bool FTPClient::change_directory(const char *directory_name) {
    // Run CWD command
    return (run_command("CWD ", directory_name) == 250);
}

// List directory
ssize_t FTPClient::list_directory(void) {
    // Open passive mode
    if (open_passive_mode()) {
        ssize_t n = -1;
        // Run LIST command
        uint16_t ret = run_command("LIST", "");
        if (ret == 150 || ret == 125) {
            // Wait for passive server answer
            uint32_t timeout = millis() + m_timeout;
            while ((!m_passive_client.available()) && (millis() < timeout))
                delay(5);

            // Cat directory list
            n = 0;
            while (m_passive_client.available()) {
                String s = m_passive_client.readStringUntil('\n');
                os_ftp_cat_file(s.c_str());
                n++;
            }
        }

        // Stop passive client
        // m_passive_client.stop();

        // Close passive mode
        close_passive_mode();
        return n;
    }

    // Return: error
    return -1;
}

// FTPClient private functions

// Run command
uint16_t FTPClient::run_command(const char *command, const char *param,
                                char *answer) {
    // Initialize error code
    m_last_error_code = 530;

    // Check connection status
    if (m_client.connected()) {
        if (os_debug()) {
            hal_print_string(command);
            hal_print_string(param);
            hal_print_string("\r\n");
        }

        // Send command
        m_client.print(command);
        m_client.println(param);

        // Return command result
        return get_server_answer(answer);
    }

    // Return: error
    return m_last_error_code;
}

void FTPClient::flush_available() {
    int available = m_client.available();
    while (available > 0) {
        uint8_t buffer[FTP_CLIENT_BUFFER_SIZE];
        available = available > FTP_CLIENT_BUFFER_SIZE ? FTP_CLIENT_BUFFER_SIZE : available;
        m_client.read(buffer, available);
        available = m_client.available();
    }
}

// Get answer from server
uint16_t FTPClient::get_server_answer(char *answer) {
    // Wait for server answer
    uint32_t timeout = millis() + m_timeout;
    while ((!m_client.available()) && (millis() < timeout))
        delay(5);

    // Initialize error code
    m_last_error_code = 530;

    // Check connection status
    if (m_client.available()) {
        // Read server answer
        char buffer[FTP_CLIENT_BUFFER_SIZE];
        uint8_t buffer_count = 0;
        while (m_client.available()) {
            // Get byte recieved from server
            char byte_received = m_client.read();

            // Store byte in buffer
            if (buffer_count < FTP_CLIENT_BUFFER_SIZE) {
                buffer[buffer_count++] = byte_received;
                buffer[buffer_count] = '\0';
            }

            // FIXME: Problem with 220 (can have CRLF chars)
            if (byte_received == '\n')
                break;

            // Delay
            delay(5);
        }

        if (os_debug()) {
            hal_print_string(buffer);
        }

        // Copy answer
        if (answer != NULL)
            strcpy(answer, &buffer[4]);

        // Set error code
        m_last_error_code = atoi(buffer);
    }

    // Return error code
    return m_last_error_code;
}

// Open passive mode
bool FTPClient::open_passive_mode() {
    // Run PASV command
    char buffer[FTP_CLIENT_BUFFER_SIZE];
    if (run_command("PASV", "", buffer) == 227) {
        // Initialize error code
        m_last_error_code = 530;

        // Extract data from buffer
        if (strtok(buffer, "(,")) {
            uint8_t data[6];
            for (uint8_t i = 0; i < 6; i++) {
                char *token = strtok(NULL, "(,");
                if (token == NULL)
                    return false;
                data[i] = atoi(token);
            }

            // Connect to passive server
            return m_passive_client.connect(
                IPAddress(data[0], data[1], data[2], data[3]),
                (data[4] << 8) | (data[5] << 0));
        }
    }

    // Return
    return false;
}

// Close passive mode
bool FTPClient::close_passive_mode() {
    // Stop passive client
    m_passive_client.stop();

    // Return
    uint16_t ret = get_server_answer();
    return (ret == 226);
}

// Receive file
void FTPClient::receive(File &destination_file, ssize_t size) {
    if (os_debug())
        hal_print_integer("*** Receiving %zd bytes\r\n", size);

    // Read all data from passive server
    uint8_t block[FTP_CLIENT_TRANSFER_BLOCK_SIZE];
    ssize_t read_size = 0;
    while (read_size < size) {
        int available = m_passive_client.available();
        if (available == 0) {
            delay(5);
            continue;
        }

        if (available > FTP_CLIENT_TRANSFER_BLOCK_SIZE)
            available = FTP_CLIENT_TRANSFER_BLOCK_SIZE;

        size_t block_size = m_passive_client.readBytes(block, available);
        if (block_size > 0) {
            destination_file.write(block, block_size);
            read_size += block_size;
        }
    }

    if (os_debug())
        hal_print_string("*** Received end\r\n");
}

// Send file
void FTPClient::send(File &source_file) {
    // Write buffer block by block
    uint8_t block[FTP_CLIENT_TRANSFER_BLOCK_SIZE];
    uint32_t block_size = 0;
    while (source_file.available()) {
        block[block_size++] = source_file.read();
        if (block_size == FTP_CLIENT_TRANSFER_BLOCK_SIZE) {
            m_passive_client.write(block, block_size);
            block_size = 0;
        }
    }
    if (block_size > 0)
        m_passive_client.write(block, block_size);
}
