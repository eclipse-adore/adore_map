/********************************************************************************
 * Copyright (c) 2026 Contributors to the Eclipse Foundation
 *
 * See the NOTICE file(s) distributed with this work for additional
 * information regarding copyright ownership.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License 2.0 which is available at
 * https://www.eclipse.org/legal/epl-2.0
 *
 * SPDX-License-Identifier: EPL-2.0
 ********************************************************************************/

#pragma once
#include <curl/curl.h>
#include <memory>
#include <string>

/**
 *  @brief A wrapper class for cURL to manage initialization and cleanup in a RAII manner.
 *  @details This class encapsulates a cURL easy handle and manages its lifecycle. It also handles
 *           global initialization and cleanup of the cURL library if specified. The class provides
 *           methods to set common cURL options and a write callback for handling response data.
 */
class CurlWrapper {
public:

  /** @brief Factory method to create a CurlWrapper instance
   * @param[in] global_init If true, performs global cURL initialization
   * @param[in] global_cleanup If true, will perform global cURL cleanup in the destructor
   * @param[in] debug_mode If true, enables debug output
   * @return std::unique_ptr<CurlWrapper> A unique pointer to the created CurlWrapper instance
   */
  static std::unique_ptr<CurlWrapper> make( const bool global_init = false, const bool global_cleanup = false, 
    const bool debug_mode = false );

  /** @brief Destructor for CurlWrapper
   * @details Cleans up the cURL easy handle and performs global cleanup if specified.
   */
  ~CurlWrapper();

  /** @brief cURL write callback function
   * @param[in] ptr Pointer to the data received from cURL
   * @param[in] size Size of each data element
   * @param[in] nmemb Number of data elements
   * @param[out] userdata Pointer to the user data (in this case, a std::string)
   * @return Number of bytes written
   */
  static size_t write_callback( char* ptr, size_t size, size_t nmemb, void* userdata );

  /** @brief Sets general cURL options including authentication and write callback
   * @param[in] username The username for authentication
   * @param[in] password The password for authentication
   * @return CURLcode indicating the result of setting options
   */
  CURLcode set_general_options( const std::string& username, const std::string& password );

  /** @brief Sets the URL for the cURL request
   * @param[in] url The URL to set
   * @return CURLcode indicating the result of setting the URL
   */
  CURLcode set_url( const std::string& url );

  /** @brief Performs the cURL request
   * @return CURLcode indicating the result of the perform operation
   */
  CURLcode perform();

  /** @brief Downloads data from the specified URL
   * @param[in] url The URL to download data from
   * @return CURLcode indicating the result of the download operation
   */
  CURLcode download( const std::string& url );

  // Getters
  
  inline std::string& get_read_buffer() { return read_buffer; }
  inline CURL* get_curl() { return curl; }

private:

  /** @brief Private constructor for CurlWrapper
   */
  explicit CurlWrapper( CURL* curl, const bool global_cleanup = false, const bool debug = false );
  CURL* curl;
  std::string read_buffer;
  const bool global_cleanup;
  const bool debug_mode;
};
