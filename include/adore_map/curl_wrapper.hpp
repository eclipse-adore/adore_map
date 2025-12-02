/********************************************************************************
 * Copyright (c) 2025 Contributors to the Eclipse Foundation
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
#include <mutex>
#include <iostream>

/**
 *  @brief A wrapper class for cURL to manage initialization and cleanup in a RAII manner.
 *  @details This class encapsulates a cURL easy handle and manages its lifecycle. It also handles
 *           global initialization and cleanup of the cURL library if specified. The class provides
 *           methods to set common cURL options and a write callback for handling response data.
 */
class CurlWrapper {
public:

  /** @brief Factory method to create a CurlWrapper instance
   * @param global_init If true, performs global cURL initialization
   * @param global_cleanup If true, will perform global cURL cleanup in the destructor
   * @param debug If true, enables debug output
   * @return std::unique_ptr< CurlWrapper > A unique pointer to the created CurlWrapper instance
   */
  static std::unique_ptr<CurlWrapper> make( const bool global_init = false, const bool global_cleanup = false, 
    const bool debug_mode = false ) 
  {
    if( global_init )
    {
      static std::once_flag once_flag;
      std::call_once( once_flag, 
        [ debug_mode ]()
        {
          if( debug_mode )
          {
            std::cout << "CurlWrapper::make: Performing one time global curl initialization..." << std::endl;
          }
          auto result = curl_global_init( CURL_GLOBAL_DEFAULT );
          if( debug_mode )
          {
            if( result == CURLE_OK )
            {
              std::cout << "CurlWrapper::make: Global curl initialization done." << std::endl;
            }
            else
            {
              std::cerr << "CurlWrapper::make: Global curl initialization failed: " 
                + std::string( curl_easy_strerror( result ) ) << std::endl;
            }
          }
        } );
    }
    if( debug_mode )
    {
      std::cout << "CurlWrapper::make: Creating curl instance..." << std::endl;
    }
    CURL* curl = curl_easy_init();
    if( debug_mode )
    {
      if( curl )
      {
        std::cout << "CurlWrapper::make: curl instance created." <<  std::endl;
      }
      else
      {
        std::cerr << "CurlWrapper::make: Failed to create curl instance." <<  std::endl;
      }
    }
    if( !curl )
    {
      return nullptr;
    }
    return std::unique_ptr<CurlWrapper>( new CurlWrapper( curl, global_cleanup, debug_mode ) );
  }

  /** @brief Destructor for CurlWrapper
   * @details Cleans up the cURL easy handle and performs global cleanup if specified.
   */
  ~CurlWrapper() 
  {
    if( curl )
    {
      if( debug_mode )
      {
        std::cout << "CurlWrapper::~CurlWrapper: Cleaning up curl..." << std::endl;
      }
      curl_easy_cleanup( curl );
      if( debug_mode )
      {
        std::cout << "CurlWrapper::~CurlWrapper: curl cleaned up." << std::endl;
      }
    }
    if( global_cleanup )
    {
      if( debug_mode )
      {
        std::cout << "CurlWrapper::~CurlWrapper: Performing global curl cleanup..." << std::endl;
      }
      curl_global_cleanup();
      if( debug_mode )
      {
        std::cout << "CurlWrapper::~CurlWrapper: Global curl cleanup done." << std::endl;
      }
    }
  }

  /** @brief cURL write callback function
   * @param ptr Pointer to the data received from cURL
   * @param size Size of each data element
   * @param nmemb Number of data elements
   * @param userdata Pointer to the user data (in this case, a std::string)
   * @return Number of bytes written
   */
  static size_t write_callback( char* ptr, size_t size, size_t nmemb, void* userdata )
  {
    // Cast userdata to std::string pointer and append the received data
    ( (std::string*) userdata )->append( ptr, size * nmemb );
    return size * nmemb;
  }

  /** @brief Sets general cURL options including authentication and write callback
   * @param username The username for authentication
   * @param password The password for authentication
   * @return CURLcode indicating the result of setting options
   */
  CURLcode set_general_options( const std::string& username, const std::string& password ) 
  {
    CURLcode ret = curl_easy_setopt( curl, CURLOPT_WRITEFUNCTION, CurlWrapper::write_callback );
    if( ret != CURLE_OK )
    {
      std::cerr << "CurlWrapper::set_general_options: Failed to set write callback: " 
        + std::string( curl_easy_strerror( ret ) ) << std::endl;
      return ret;
    }
    ret = curl_easy_setopt( curl, CURLOPT_USERAGENT, "libcurl-agent/1.0" );
    if( ret != CURLE_OK )
    {
      std::cerr << "CurlWrapper::set_general_options: Failed to set user agent: " 
        + std::string( curl_easy_strerror( ret ) ) << std::endl;
      return ret;
    }
    ret = curl_easy_setopt( curl, CURLOPT_USERNAME, username.c_str() );
    if( ret != CURLE_OK )
    {
      std::cerr << "CurlWrapper::set_general_options: Failed to set username: " 
        + std::string( curl_easy_strerror( ret ) ) << std::endl;
      return ret;
    }
    ret = curl_easy_setopt( curl, CURLOPT_PASSWORD, password.c_str() );
    if( ret != CURLE_OK )
    {
      std::cerr << "CurlWrapper::set_general_options: Failed to set password: " 
        + std::string( curl_easy_strerror( ret ) ) << std::endl;
      return ret;
    }
    ret = curl_easy_setopt( curl, CURLOPT_WRITEDATA, &read_buffer );
    if( ret != CURLE_OK )
    {
      std::cerr << "CurlWrapper::set_general_options: Failed to set write data: " 
        + std::string( curl_easy_strerror( ret ) ) << std::endl;
    }
    return ret;
  }

  /** @brief Sets the URL for the cURL request
   * @param url The URL to set
   * @return CURLcode indicating the result of setting the URL
   */
  CURLcode set_url( const std::string& url ) 
  {
    CURLcode ret = curl_easy_setopt( curl, CURLOPT_URL, url.c_str() );
    if( ret != CURLE_OK )
    {
      std::cerr << "CurlWrapper::set_url: Failed to set URL: " 
        + std::string( curl_easy_strerror( ret ) ) << std::endl;
    }
    return ret;
  }

  /** @brief Performs the cURL request
   * @return CURLcode indicating the result of the perform operation
   */
  CURLcode perform() 
  {
    CURLcode ret = curl_easy_perform( curl );
    if( ret != CURLE_OK )
    {
      std::cerr << "CurlWrapper::perform: cURL error: " 
        + std::string( curl_easy_strerror( ret ) ) << std::endl;
    }
    return ret;
  }

  /** @brief Downloads data from the specified URL
   * @param url The URL to download data from
   * @return CURLcode indicating the result of the download operation
   */
  CURLcode download( const std::string& url ) 
  {
    CURLcode ret = set_url( url );
    if( ret != CURLE_OK )
    {
      return ret;
    }
    read_buffer.clear(); // Clear previous data
    ret = perform();
    if( ret != CURLE_OK )
    {
      return ret;
    }
    if( read_buffer.empty() )
    {
      std::cerr << "CurlWrapper::download: No data received from server for URL: " << url << std::endl;
      return CURLE_RECV_ERROR; // Return an error if no data was received
    }

    return CURLE_OK;
  }

  // Getters
  
  inline std::string& get_read_buffer() { return read_buffer; }
  inline CURL* get_curl() { return curl; }

private:

  /** @brief Private constructor for CurlWrapper
   */
  explicit CurlWrapper( CURL* curl, const bool global_cleanup = false, const bool debug = false ) : 
      curl( curl ), global_cleanup( global_cleanup ), debug_mode( debug ) {}
  CURL* curl;
  std::string read_buffer;
  const bool global_cleanup;
  const bool debug_mode;
};
