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
#include "adore_map/map_downloader.hpp"
#include "adore_map/json_file_helpers.hpp"

// Parameterized constructor for MapDownloader
MapDownloader::MapDownloader( const std::string& server_url, const std::string& username, 
  const std::string& password, const std::string& project_name, const BoundingBox& bounding_box, 
  const std::string& file_cache_path, const bool curl_global_init, const bool curl_global_cleanup, 
  const bool debug ) : server_url( server_url ), project_name( project_name ), bounding_box( bounding_box ), 
  debug_mode( debug )
{
  map_cache.set_up_file_cache_path( file_cache_path );
  map_cache.set_debug_mode( debug );
  curl_wrapper = CurlWrapper::make( curl_global_init, curl_global_cleanup, debug_mode );
  if( curl_wrapper )
  {
    curl_wrapper->set_general_options( username, password );
  }
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

// Downloads map data for a specific layer within a bounding box (flexible version)
bool MapDownloader::download_map( const std::string& server_url, const std::string& project_name, 
  const std::string& layer_name, const BoundingBox& bounding_box )
{
  return download_map_as_json( server_url, project_name, layer_name, bounding_box );
}

// Downloads map data as JSON for a specific layer (private version)
bool MapDownloader::download_map_as_json( const std::string& layer_name )
{
  return download_map_as_json( server_url, project_name, layer_name, bounding_box );
}

// Downloads map data as JSON for a specific layer within a bounding box (private version)
bool MapDownloader::download_map_as_json( const std::string& layer_name, const BoundingBox& bounding_box )
{
  return download_map_as_json( server_url, project_name, layer_name, bounding_box );
}

// Downloads map data as JSON (private flexible version)
bool MapDownloader::download_map_as_json( const std::string& server_url, 
  const std::string& project_name, const std::string& layer_name, const BoundingBox& bounding_box )
{
  std::string url_key = server_url + project_name + "/" + layer_name + "&" + bounding_box.to_string();
  // Check if the map is already in the cache
  auto cached_map = map_cache.try_get( url_key );
  if( cached_map != nullptr )
  {
    // If the map is found in the cache, load it from there
    json_data = *cached_map;
    if( debug_mode ) 
    {
      // Debugging line to see the key being requested from cache
      std::cout << "MapDownloader::download_map_as_json: Map found in cache for key: " << url_key << std::endl;
      // Debugging line to see the JSON data being pretty printed
      std::cout << "MapDownloader::download_map_as_json: Pretty printing cached map data." << std::endl;
      pretty_print_map( json_data );
    }
    return true;
  }
  // Loading a map as JSON from a WFS (Web Feature Service) server
  // Using cURL to perform the HTTP request and retrieve the JSON data
  if( curl_wrapper ) 
  {
    assert( curl_wrapper->get_curl() != nullptr ); // by this point curl must be initialized
    // Set cURL options
    std::string url = server_url + project_name + "/ows?service=WFS&version=1.0.0&request=GetFeature&typeName=" 
      + layer_name + "&outputFormat=application/json" + bounding_box.to_query_string();
    if( debug_mode ) 
    {
      // Debugging line to see the constructed URL
      std::cout << "MapDownloader::download_map_as_json: Constructed URL: " << url << std::endl;
    }
    if( curl_wrapper->download( url ) != CURLE_OK )
    {
      std::cerr << "MapDownloader::download_map_as_json: cURL download failed for URL: " << url << std::endl;
      return false;
    }
    parse_json(); // parse into member json_data
    // Put the map into the cache
    map_cache.put( url_key, json_data );
    if( debug_mode ) 
    {
      // Debugging line to see the key being saved to cache
      std::cout << "MapDownloader::download_map_as_json: Map put into cache for key: " << url_key << std::endl;
    }
    // Since the map was successfully downloaded and parsed, return true
    return true;
  }
  std::cerr << "MapDownloader::download_map_as_json: cURL wrapper and cURL are not initialized." << std::endl;
  return false; // Return false if cURL is not initialized
}

// Unloads the map data from memory
void MapDownloader::unload_map()
{
  curl_wrapper->get_read_buffer().clear(); // Clear the read buffer
  json_data.clear(); // Clear the JSON data as well
}

// Unloads the map data from memory (more flexible version)
void MapDownloader::unload_map( nlohmann::json& json_data )
{
  curl_wrapper->get_read_buffer().clear(); // Clear the read buffer
  json_data.clear(); // Clear the JSON data as well
}

// Parses JSON data from the internal read buffer and populates the internal JSON data object
void MapDownloader::parse_json()
{
  parse_json( curl_wrapper->get_read_buffer(), json_data );
}

// Parses JSON data from a string and populates the internal JSON data object
void MapDownloader::parse_json( const std::string& json_str )
{
  parse_json( json_str, json_data );
}

// Parses JSON data from the internal read buffer and populates the provided JSON data object
void MapDownloader::parse_json( nlohmann::json& json_data )
{
  parse_json( curl_wrapper->get_read_buffer(), json_data );
}

// Parses JSON data from a string
// Populates the provided json_data object with the parsed data
void MapDownloader::parse_json( const std::string& json_str, nlohmann::json& json_data )
{
  json_data = nlohmann::json::parse( json_str );
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
  JsonFileHelpers::save( json_data, filename, "MapDownloader::save_map(json_data, filename)" );
}

// Saves the map data stored in json_data to a file
void MapDownloader::save_json( const std::string& filename )
{
  JsonFileHelpers::save( json_data, filename, "MapDownloader::save_json(filename)" );
}

// Loads the map data from a file into member variable json_data
void MapDownloader::load_map( const std::string& filename )
{
  JsonFileHelpers::load( filename, json_data, "MapDownloader::load_map(filename)" );
}

// Loads the map data from a file into a given JSON object
void MapDownloader::load_map( const std::string& filename, nlohmann::json& json_data )
{
  JsonFileHelpers::load( filename, json_data, "MapDownloader::load_map(filename, json_data)" );
}

// Delegatory methods to turn off and on the cache
// These methods call the corresponding methods on the MapCache instance
// to disable or enable caching of map data, respectively

// Turns off the map cache
void MapDownloader::turn_off_cache()
{
  map_cache.turn_off();
}

// Turns on the map cache
void MapDownloader::turn_on_cache()
{
  map_cache.turn_on();
}
