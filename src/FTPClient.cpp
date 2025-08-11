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

// FTPClient public functions

// Constructor
FTPClient::FTPClient(uint16_t timeout) : m_timeout(timeout),
                                         m_last_error_code(0)
{
}

// Open connection
bool FTPClient::open(const char *server_address, const uint16_t server_port, const char *user_name, const char *user_password)
{
  // Connect to server
  if (m_client.connect(server_address, server_port))
  {
    // Check if server is ready for a new user
    if (get_server_answer() == 220)
    {
      // Run USER command
      if (run_command("USER ", user_name) == 331)
      {
        // Run PASS command
        return (run_command("PASS ", user_password) == 230);
      }
    }
  }

  // Return: error
  return false;
}

// Close connection
void FTPClient::close()
{
  // Run QUIT command
  run_command("QUIT");

  // Close WiFi client
  m_client.stop();
}

// Write data to file
bool FTPClient::write_file(const char *file_name, uint8_t *buffer, size_t buffer_size)
{
  // Open passive mode
  if (open_passive_mode())
  {
    // Run STOR command
    if (run_command("STOR ", file_name) == 150)
    {
      // Send buffer
      send(buffer, buffer_size);
    }

    // Close passive mode
    return close_passive_mode();
  }

  // Return: error
  return false;
}

// Write data to file
bool FTPClient::write_file(const char *file_name, const char *spiffs_file_name)
{
  // Mount file system
  if (LittleFS.begin())
  {
    // Open source file
    File source_file = LittleFS.open(spiffs_file_name, "r");
    if (source_file)
    {
      // Open passive mode
      if (open_passive_mode())
      {
        // Run STOR command
        if (run_command("STOR ", file_name) == 150)
        {
          // Send file
          send(source_file);
        }

        // Close passive mode
        return close_passive_mode();
      }
    }
  }

  // Return: error
  return false;
}

// Append data to file
bool FTPClient::append_file(const char *file_name, uint8_t *buffer, size_t buffer_size)
{
  // Open passive mode
  if (open_passive_mode())
  {
    // Run APPE command
    if (run_command("APPE ", file_name) == 150)
    {
      // Send buffer
      send(buffer, buffer_size);
    }

    // Close passive mode
    return close_passive_mode();
  }

  // Return: error
  return false;
}

// Append data to file
bool FTPClient::append_file(const char *file_name, const char *spiffs_file_name)
{
  // Mount file system
  if (LittleFS.begin())
  {
    // Open source file
    File source_file = LittleFS.open(spiffs_file_name, "r");
    if (source_file)
    {
      // Open passive mode
      if (open_passive_mode())
      {
        // Run APPE command
        if (run_command("APPE ", file_name) == 150)
        {
          // Send file
          send(source_file);
        }

        // Close passive mode
        return close_passive_mode();
      }
    }
  }

  // Return: error
  return false;
}

// Read file
bool FTPClient::read_file(const char *file_name, uint8_t *buffer, size_t buffer_size)
{
  // Open passive mode
  if (open_passive_mode())
  {
    // Run RETR command
    if (run_command("RETR ", file_name) == 150)
    {
      // Receive buffer
      receive(buffer, buffer_size);
    }

    // Close passive mode
    return close_passive_mode();
  }

  // Return: error
  return false;
}

// Read file
bool FTPClient::read_file(const char *file_name, const char *spiffs_file_name)
{
  // Mount file system
  if (LittleFS.begin())
  {
    // Open destination file
    File destination_file = LittleFS.open(spiffs_file_name, "w");
    if (destination_file)
    {
      // Open passive mode
      if (open_passive_mode())
      {
        // Run RETR command
        if (run_command("RETR ", file_name) == 150)
        {
          // Receive file
          receive(destination_file);
        }

        // Close passive mode
        return close_passive_mode();
      }
    }
  }

  // Return: error
  return false;
}

// Rename file
bool FTPClient::rename_file(const char *from, const char *to)
{
  // Run RNFR command
  if (run_command("RNFR ", from) == 350)
  {
    // Run RNTO command
    return (run_command("RNTO ", to) == 250);
  }

  // Return: error
  return false;
}

// Delete file
bool FTPClient::delete_file(const char *file_name)
{
  // Run DELE command
  return (run_command("DELE ", file_name) == 250);
}

// Get last modified time
bool FTPClient::get_last_modified_time(const char *file_name, char *result)
{
  // Run MDTM command
  return (run_command("MDTM ", file_name, result) == 213);
}

// Create directory
bool FTPClient::create_directory(const char *directory_name)
{
  // Run MKD command
  return (run_command("MKD ", directory_name) == 257);
}

// Remove directory
bool FTPClient::remove_directory(const char *directory_name)
{
  // Run RMD command
  return (run_command("RMD ", directory_name) == 250);
}

// Change directory
bool FTPClient::change_directory(const char *directory_name)
{
  // Run CWD command
  return (run_command("CWD ", directory_name) == 250);
}

// List directory
bool FTPClient::list_directory(const char *directory_name, String *item_list, uint8_t item_list_count)
{
  // Open passive mode
  if (open_passive_mode())
  {
    // Run MLSD command
    if (run_command("MLSD ", directory_name) == 150)
    {
      // Wait for passive server answer
      uint32_t timeout = millis() + m_timeout;
      while ((!m_passive_client.available()) && (millis() < timeout))
        delay(5);

      // Get directory list
      uint8_t item_count = 0;
      while (m_passive_client.available())
        if (item_count < item_list_count)
          item_list[item_count++] = m_passive_client.readStringUntil('\n');
    }

    // Close passive mode
    return close_passive_mode();
  }

  // Return: error
  return false;
}

// FTPClient private functions

// Run command
uint16_t FTPClient::run_command(const char *command, const char *param, char *answer)
{
  // Initialize error code
  m_last_error_code = 530;

  // Check connection status
  if (m_client.connected())
  {
    // Send command
    m_client.print(command);
    m_client.println(param);

    // Return command result
    return get_server_answer(answer);
  }

  // Return: error
  return m_last_error_code;
}

// Get answer from server
uint16_t FTPClient::get_server_answer(char *answer)
{
  // Wait for server answer
  uint32_t timeout = millis() + m_timeout;
  while ((!m_client.available()) && (millis() < timeout))
    delay(5);

  // Initialize error code
  m_last_error_code = 530;

  // Check connection status
  if (m_client.available())
  {
    // Read server answer
    char buffer[FTP_CLIENT_BUFFER_SIZE];
    uint8_t buffer_count = 0;
    while (m_client.available())
    {
      // Get byte recieved from server
      char byte_received = m_client.read();

      // Store byte in buffer
      if (buffer_count < FTP_CLIENT_BUFFER_SIZE)
      {
        buffer[buffer_count++] = byte_received;
        buffer[buffer_count] = '\0';
      }

      // Delay
      delay(5);
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
bool FTPClient::open_passive_mode()
{
  // Run MLSD command
  char buffer[FTP_CLIENT_BUFFER_SIZE];
  if (run_command("PASV", "", buffer) == 227)
  {
    // Initialize error code
    m_last_error_code = 530;

    // Extract data from buffer
    if (strtok(buffer, "(,"))
    {
      uint8_t data[6];
      for (uint8_t i = 0; i < 6; i++)
      {
        char *token = strtok(NULL, "(,");
        if (token == NULL)
          return false;
        data[i] = atoi(token);
      }

      // Connect to passive server
      return m_passive_client.connect(IPAddress(data[0], data[1], data[2], data[3]), (data[4] << 8) | (data[5] << 0));
    }
  }

  // Return
  return false;
}

// Close passive mode
bool FTPClient::close_passive_mode()
{
  // Stop passive client
  m_passive_client.stop();

  // Return
  return (get_server_answer() == 226);
}

// Receive buffer
void FTPClient::receive(uint8_t *buffer, size_t buffer_size)
{
  // Wait for passive server answer
  uint32_t timeout = millis() + m_timeout;
  while ((!m_passive_client.available()) && (millis() < timeout))
    delay(5);

  // Read all data from passive server
  uint8_t data_byte;
  uint8_t data_byte_count = 0;
  while (m_passive_client.available())
  {
    m_passive_client.readBytes((uint8_t *)&data_byte, 1);
    if (data_byte_count < buffer_size)
      buffer[data_byte_count++] = data_byte;
  }
}

// Receive file
void FTPClient::receive(File &destination_file)
{
  // Wait for passive server answer
  uint32_t timeout = millis() + m_timeout;
  while ((!m_passive_client.available()) && (millis() < timeout))
    delay(5);

  // Read all data from passive server
  uint8_t block[FTP_CLIENT_TRANSFER_BLOCK_SIZE];
  while (m_passive_client.available())
  {
    size_t block_size = m_passive_client.readBytes(block, FTP_CLIENT_TRANSFER_BLOCK_SIZE);
    if (block_size > 0)
      destination_file.write(block, block_size);
  }
}

// Send buffer
void FTPClient::send(uint8_t *buffer, size_t buffer_size)
{
  // Write buffer block by block
  uint8_t block[FTP_CLIENT_TRANSFER_BLOCK_SIZE];
  uint32_t block_size = 0;
  for (uint32_t i = 0; i < buffer_size; i++)
  {
    block[block_size++] = buffer[i];
    if (block_size == FTP_CLIENT_TRANSFER_BLOCK_SIZE)
    {
      m_passive_client.write(block, block_size);
      block_size = 0;
    }
  }
  if (block_size > 0)
    m_passive_client.write(block, block_size);
}

// Send file
void FTPClient::send(File &source_file)
{
  // Write buffer block by block
  uint8_t block[FTP_CLIENT_TRANSFER_BLOCK_SIZE];
  uint32_t block_size = 0;
  while (source_file.available())
  {
    block[block_size++] = source_file.read();
    if (block_size == FTP_CLIENT_TRANSFER_BLOCK_SIZE)
    {
      m_passive_client.write(block, block_size);
      block_size = 0;
    }
  }
  if (block_size > 0)
    m_passive_client.write(block, block_size);
}
