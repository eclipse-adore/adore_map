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

#include <iostream>
#include <string>
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <fstream>
#include "adore_map/map_cache.hpp"
#include "adore_map/map_downloader.hpp"

// Default constructor for MapDownloader
MapDownloader::MapDownloader()
  : curl( nullptr ), read_buffer( "" ), server_url( "" ), username( "" ), password( "" ), project_name( "" ), 
  bounding_box( 0.0, 0.0, 0.0, 0.0, "" ), file_cache_path( "" ), debug_mode( false ), map_cache( new MapCache( file_cache_path ) ) {}

// Parameterized constructor for MapDownloader
MapDownloader::MapDownloader( const std::string& server_url, const std::string& username, 
  const std::string& password, const std::string& project_name, const BoundingBox& bounding_box, 
  std::string file_cache_path, const bool debug) : curl( nullptr ), read_buffer( "" ), server_url( server_url ), 
  username( username ), password( password ), project_name( project_name ), bounding_box( bounding_box ), 
  file_cache_path( file_cache_path ), debug_mode( debug ), map_cache( new MapCache( file_cache_path, 64, 256, true, 
    debug ) ) {}

// Destructor for MapDownloader
MapDownloader::~MapDownloader() 
{
  cleanup();
  unload_map();
  delete map_cache; // Clean up the MapCache instance
}

// cURL write callback function
size_t MapDownloader::write_callback( char* ptr, size_t size, size_t nmemb, void* userdata )
{
  // Cast userdata to std::string pointer and append the received data
  // ptr is a pointer to the data received from cURL
  // size is the size of each element, nmemb is the number of elements
  ( (std::string*) userdata )->append( ptr, size * nmemb );
  return size * nmemb;
}

// Initializes the MapDownloader instance by setting up cURL
bool MapDownloader::initialize() {
  return initialize_curl( username, password, &curl, &read_buffer ) == CURLE_OK;
}

// Static method to initialize cURL
CURLcode MapDownloader::initialize_curl( const std::string& username, const std::string& password, CURL **curl, 
  std::string* read_buffer )
{
  // Initialize global cURL environment
  curl_global_init( CURL_GLOBAL_ALL );

  // Initialize cURL
  *curl = curl_easy_init();
  if( !*curl )
  {
    // Something went wrong
    curl_global_cleanup();
    return CURLE_FAILED_INIT; // Return an error code if cURL initialization failed
  }

  curl_easy_setopt( *curl, CURLOPT_WRITEFUNCTION, MapDownloader::write_callback );
  curl_easy_setopt( *curl, CURLOPT_USERAGENT, "libcurl-agent/1.0" );
  curl_easy_setopt( *curl, CURLOPT_USERNAME, username.c_str() );
  curl_easy_setopt( *curl, CURLOPT_PASSWORD, password.c_str() );
  curl_easy_setopt( *curl, CURLOPT_WRITEDATA, read_buffer );
  return CURLE_OK; // Return success code
}

// Cleans up the MapDownloader instance by releasing cURL resources
void MapDownloader::cleanup()
{
  curl_cleanup( curl );
  curl = nullptr;
}

// Static method to clean up cURL resources
void MapDownloader::curl_cleanup( CURL *curl )
{
  if( curl ) {
    // This function cleans up the cURL environment
    // It is called after the cURL operations are done
    curl_easy_cleanup(curl);
  }
  curl_global_cleanup();
}

// Downloads map data for a specific layer
bool MapDownloader::download_map( const std::string& layer_name )
{
  return download_map_as_json( layer_name );
}

// Downloads map data for a specific layer within a bounding box
bool MapDownloader::download_map( const std::string& layer_name, const BoundingBox& bounding_box )
{
  return download_map_as_json( layer_name, bounding_box );
}

// Downloads map data as JSON using cURL
CURLcode MapDownloader::download_map( CURL *curl, const std::string& server_url, const std::string& project_name, 
  const std::string& layer_name, const BoundingBox& bounding_box, std::string* read_buffer )
{
  return download_map_as_json( curl, server_url, project_name, layer_name, bounding_box, read_buffer );
}

// Downloads map data as JSON for a specific layer
bool MapDownloader::download_map_as_json( const std::string& layer_name )
{
  return download_map_as_json( curl, server_url, project_name, layer_name, bounding_box, &read_buffer ) == CURLE_OK;
}

// Downloads map data as JSON for a specific layer within a bounding box
bool MapDownloader::download_map_as_json( const std::string& layer_name, const BoundingBox& bounding_box )
{
  return download_map_as_json( curl, server_url, project_name, layer_name, bounding_box, &read_buffer ) == CURLE_OK;
}

// Downloads map data as JSON using cURL
CURLcode MapDownloader::download_map_as_json( CURL *curl, const std::string& server_url, 
  const std::string& project_name, const std::string& layer_name, const BoundingBox& bounding_box, 
  std::string* read_buffer )
{
  if( read_buffer == nullptr )
  {
    throw std::invalid_argument( "MapDownloader::download_map_as_json: Null read_buffer passed." );
  }
  std::string url_key = server_url + project_name + "/" + layer_name + "&" + bounding_box.to_string();
  // Check if the map is already in the cache
  if( map_cache != nullptr )
  {
    auto cached_map = map_cache->try_get( url_key );
    if( cached_map.second )
    {
      // If the map is found in the cache, load it from there
      json_data = *cached_map.first;
      if( debug_mode ) 
      {
        // Debugging line to see the key being requested from cache
        std::cout << "MapDownloader::download_map_as_json: Map found in cache for key: " << url_key << std::endl;
        // Debugging line to see the JSON data being pretty printed
        std::cout << "MapDownloader::download_map_as_json: Pretty printing cached map data." << std::endl;
        pretty_print_map( json_data );
      }
      return CURLE_OK; // Return success code
    }
  }
  // Loading a map as JSON from a WFS (Web Feature Service) server
  // Using cURL to perform the HTTP request and retrieve the JSON data
  if( curl )
  {
    // Set cURL options
    std::string url = server_url + project_name + "/ows?service=WFS&version=1.0.0&request=GetFeature&typeName=" 
      + layer_name + "&outputFormat=application/json" + bounding_box.to_query_string();
    if( debug_mode ) 
    {
      // Debugging line to see the constructed URL
      std::cout << "MapDownloader::download_map_as_json: Constructed URL: " << url << std::endl;
    }
    curl_easy_setopt( curl, CURLOPT_URL, url.c_str() );
    CURLcode res = curl_easy_perform( curl );
    if( res != CURLE_OK )
    {
      std::cerr << "MapDownloader::download_map_as_json: cURL error: " << curl_easy_strerror( res ) << std::endl;
      return res; // Return the error code if cURL operation failed
    }
    if( read_buffer->empty() )
    {
      std::cerr << "MapDownloader::download_map_as_json: No data received from server for URL: " << url << std::endl;
      return CURLE_RECV_ERROR; // Return an error if no  data was received
    }
    // Parse the JSON data from the read buffer
    parse_json( *read_buffer, json_data );
    // Put the map into the cache
    map_cache->put( url_key, json_data );
    // Clear the read buffer for the next request
    read_buffer->clear();
    if( debug_mode ) 
    {
      // Debugging line to see the key being saved to cache
      std::cout << "MapDownloader::download_map_as_json: Map put into cache for key: " << url_key << std::endl;
    }
    // If the map was successfully downloaded and parsed, return success code
    return res;
  }
  return CURLE_FAILED_INIT; // Return an error if cURL is not initialized
}

// Unloads the map data from memory
void MapDownloader::unload_map()
{
  read_buffer.clear(); // Clear the read buffer
  json_data.clear(); // Clear the JSON data as well
}

// Unloads the map data from memory 
void MapDownloader::unload_map( std::string* read_buffer, nlohmann::json& json_data )
{
  if( read_buffer )
  {
    read_buffer->clear(); // Clear the read buffer
  }
  json_data.clear(); // Clear the JSON data as well
}

// Parses JSON data from a string
void MapDownloader::parse_json( const std::string& json_str, nlohmann::json& json_data )
{
  if( json_data.empty() )
  {
    json_data = nlohmann::json::parse( json_str );
  }
}

// Pretty prints the map data stored in json_data
void MapDownloader::pretty_print_map()
{
  pretty_print_map( json_data );
}

// Pretty prints the map data from a given JSON object
void MapDownloader::pretty_print_map( const nlohmann::json& json_data )
{
    if( json_data.empty() )
    {
        std::cerr << "MapDownloader::pretty_print_map: No map data to pretty print." << std::endl;
        return;
    }
    std::cout << json_data.dump( 4 ) << std::endl; // pretty print the json data
}

// Saves the map data stored in json_data to a file
void MapDownloader::save_map( const std::string& filename )
{
  save_json( filename );
}

// Saves the map data from a given JSON object to a file
void MapDownloader::save_map( const nlohmann::json& json_data, const std::string& filename )
{
  save_json( json_data, filename );
}

// Saves the map data from a JSON string to a file
void MapDownloader::save_map( const std::string& json_str, const std::string& filename )
{
  nlohmann::json json_data;
  parse_json( json_str, json_data );
  save_json( json_data, filename );
}

// Saves the map data stored in json_data to a file
void MapDownloader::save_json( const std::string& filename )
{
  save_json( json_data, filename );
}

// Saves the map data from a given JSON object to a file
void MapDownloader::save_json( const nlohmann::json& json_data, const std::string& filename )
{
  std::ofstream file( filename );
  if( file.is_open() )
  {
    file << json_data.dump();
    file.close();
  }
  else
  {
    std::cerr << "MapDownloader::save_json: Failed to create JSON file: " << filename << std::endl;
  }
}

// Loads the map data from a file into member variable json_data
void MapDownloader::load_map( const std::string& filename )
{
  load_map( filename, json_data );
}

// Loads the map data from a file into a given JSON object
void MapDownloader::load_map( const std::string& filename, nlohmann::json& json_data )
{
  std::ifstream file( filename );
  if( file.is_open() )
  {
    file >> json_data;
    file.close();
  } 
  else
  {
    std::cerr << "MapDownloader::load_map: Failed to open JSON file: " << filename << std::endl;
  }
}

// Delegatory methods to turn off and on the cache
// These methods call the corresponding methods on the MapCache instance
// to disable or enable caching of map data, respectively

// Turns off the map cache
void MapDownloader::turn_off_cache()
{
  if( map_cache == nullptr )
  {
    std::cerr << "MapDownloader::turn_off_cache(): MapCache is not initialized, cannot turn off cache." << std::endl;
    return;
  }
  map_cache->turn_off();
}

// Turns on the map cache
void MapDownloader::turn_on_cache()
{
    if( map_cache == nullptr ) {
      std::cerr << "MapDownloader::turn_ofn_cache(): MapCache is not initialized, cannot turn on cache." << std::endl;
      return;
    }
    map_cache->turn_on();
}