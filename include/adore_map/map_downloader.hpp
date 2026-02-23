
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
#include <string>
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include "adore_map/map_cache.hpp"
#include "adore_map/curl_wrapper.hpp"
#include "adore_map/config.hpp"
#include "adore_map/bounding_box.hpp"

/**
 * @brief Class with caching capabilities for downloading map data from a WFS server 
 * @details This class provides methods to download map data from a specified server and project,
 *          with support for caching the downloaded data both in memory and on disk. It uses cURL 
 *          for HTTP requests and nlohmann::json for handling JSON data. The class allows for easy 
 *          retrieval of map data in JSON format.
 */
class MapDownloader
{
public:

  /** @brief Constructor for MapDownloader using a configuration object
   * @param[in] cfg Configuration object containing server and project details
   * @param[in] file_cache_path Path to the directory for caching map layer data
   */
  MapDownloader( const Config& cfg, const std::string& file_cache_path = "" );

  /** @brief Parameterized constructor for MapDownloader
   * @param[in] server_url URL of the map server
   * @param[in] username Username for authentication
   * @param[in] password Password for authentication
   * @param[in] project_name Name of the project on the map server
   * @param[in] srs_name Name of the spatial reference system (e.g., "EPSG:25832")
   * @param[in] bounding_box Bounding box for the map layer data request
   * @param[in] file_cache_path Path to the directory for caching map layer data
   * @param[in] curl_global_init Boolean flag to indicate whether to perform global cURL initialization
   * @param[in] curl_global_cleanup Boolean flag to indicate whether to perform global cURL cleanup in the destructor
   * @param[in] debug_mode Boolean flag to enable or disable debug mode
   */
  MapDownloader( const std::string& server_url, const std::string& username, const std::string& password, 
      const std::string& project_name, const std::string& srs_name, const BoundingBox& bounding_box, 
      const std::string& file_cache_path = "", const bool curl_global_init = false, 
      const bool curl_global_cleanup = false, const bool debug_mode = false );

  /** @brief Downloads data for a specific map layer
   * @param[in] layer_name Name of the layer to download
   * @return true if the download is successful, false otherwise
   */
  bool download( const std::string& layer_name );

  /** @brief Downloads data for a specific map layer within a bounding box
   * @param[in] layer_name Name of the layer to download
   * @param[in] bounding_box Bounding box for the map layer data request
   * @return true if the download is successful, false otherwise
   */
  bool download( const std::string& layer_name, const BoundingBox& bounding_box );

  /** @brief Pretty prints the map layer data stored in json_data
   */
  void pretty_print();

  /** @brief Unloads the map layer data from memory 
   * @details Clears the read buffer of the internal curl wrapper and the internal JSON data
   */
  void unload();

  /** @brief Saves the map layer data stored in json_data to a file
   * @param[in] filename The name of the file to save the map layer data to
   */
  void save( const std::string& filename );

  /** @brief Loads the map layer data from a file into member variable json_data
   * @param[in] filename The name of the file to load the map layer data from
   */
  void load( const std::string& fileName );

  /** @brief Turns off the map cache
   * @details Disables caching of map layer data in the MapCache instance
   */
  void turn_off_cache();

  /** @brief Turns on the map cache
   * @details Enables caching of map layer data in the MapCache instance
   */
  void turn_on_cache();

  /** @brief Returns whether the cache is active */
  inline const bool is_cache_active() const { return map_cache.is_cache_active(); }

  // Versions with more parameters for flexibility

   /** @brief Downloads data for a specific map layer within a bounding box
    * @param[in] server_url URL of the map server
    * @param[in] project_name Name of the project on the map server
    * @param[in] srs_name Name of the spatial reference system (e.g., "EPSG:25832")
    * @param[in] layer_name Name of the layer to download
    * @param[in] bounding_box Bounding box for the map layer data request
    * @return true if the download is successful, false otherwise
    */
  bool download( const std::string& server_url, const std::string& project_name, 
    const std::string& srs_name, const std::string& layer_name, const BoundingBox& bounding_box );

  /** @brief Unloads the map layer data from memory
   * @details Clears the internal read buffer of the curl wrapper and the given JSON data
   * @param[in] json_data JSON object to be cleared
   */
  void unload( nlohmann::json& json_data );

  /** @brief Pretty prints the map layer data from a given JSON object
   * @param[in] json_data JSON object to be pretty printed
   */
  static void pretty_print( const nlohmann::json& json_data );

  /** @brief Saves the map layer data from a given JSON object to a file
   * @param[in] json_data JSON object containing the map layer data
   * @param[in] filename Name of the file to save the map layer data to
   */
  static void save( const nlohmann::json& json_data, const std::string& filename );

  /** @brief Loads the map layer data from a file into a given JSON object
   * @param[in] filename Name of the file to load the map layer data from
   * @param[out] json_data JSON object to populate with the map layer data
   */
  static void load( const std::string& filename, nlohmann::json& json_data );
  
  // Getters for member variables

  inline const std::string& get_read_buffer() const { return curl_wrapper->get_read_buffer(); }
  inline const std::string& get_server_url() const { return server_url; }
  inline const BoundingBox& get_bounding_box() const { return bounding_box; }
  inline const std::string& get_project_name() const { return project_name; }
  inline const std::string& get_srs_name() const { return srs_name; }
  inline const nlohmann::json& get_json_data() const { return json_data; }
  inline nlohmann::json& get_json_data() { return json_data; };
  inline const MapCache& get_map_cache() const { return map_cache; }

private:

  /** @brief Downloads data as JSON for a specific map layer
   * @param[in] layer_name Name of the layer to download
   * @return true if the download is successful, false otherwise
   */
  bool download_as_json( const std::string& layer_name );

  /** @brief Downloads data as JSON for a specific layer within a bounding box
   * @param[in] layer_name Name of the layer to download
   * @param[in] bounding_box Bounding box for the map layer data request
   * @return true if the download is successful, false otherwise
   */
  bool download_as_json( const std::string& layer_name, const BoundingBox& bounding_box );

  /** @brief Downloads map layer data as JSON
   * @param[in] server_url URL of the map server
   * @param[in] project_name Name of the project on the map server
   * @param[in] srs_name Name of the spatial reference system (e.g., "EPSG:25832")
   * @param[in] layer_name Name of the layer to download
   * @param[in] bounding_box Bounding box for the map layer data request
   * @return CURLcode indicating the result of the operation
   */
  bool download_as_json( const std::string& server_url, const std::string& project_name, 
    const std::string& srs_name, const std::string& layer_name, const BoundingBox& bounding_box );

  /** @brief Parses JSON data from the internal read buffer and populates the internal JSON data object */
  void parse_json();

  /** @brief Parses JSON data from a string and populates the internal JSON data object 
   * @param[in] json_str JSON string to be parsed
   */
  void parse_json( const std::string& json_str );

  /** @brief Parses JSON data from the internal read buffer and populates the provided JSON data object 
   * @param[out] json_data JSON data object to be populated with the parsed data
   */
  void parse_json( nlohmann::json& json_data );

  /** @brief Parses JSON data from a string
   * @param[in] json_str JSON string to be parsed
   * @param[out] json_data JSON data object to be populated 
   */
  static void parse_json( const std::string& json_string, nlohmann::json& json_data );

  /** @brief Saves the map layer data stored in json_data to a file
   * @param[in] filename Name of the file to save the map data to
   */
  void save_json( const std::string& filename );
 
  std::unique_ptr<CurlWrapper> curl_wrapper;
  const std::string server_url;
  const std::string project_name;
  const std::string srs_name;
  const BoundingBox bounding_box;
  const bool debug_mode; // Flag to enable or disable debug mode
  nlohmann::json json_data;
  MapCache map_cache; // Instance of MapCache for caching map data
};
