
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
#include <string>
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include "adore_map/map_cache.hpp"
#include "adore_map/curl_wrapper.hpp"
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

  /**
   * @brief Class representing a bounding box for spatial queries
   */
  class BoundingBox
  {
  public:
    BoundingBox( double min_lat, double min_lon, double max_lat, double max_lon, const std::string& crs )
        : min_lat( min_lat ), min_lon( min_lon ), max_lat( max_lat ), max_lon( max_lon ), crs( crs ) {}
        
    // Getters for the bounding box coordinates and CRS

    inline double get_min_lat() const { return min_lat; }
    inline double get_min_lon() const { return min_lon; }
    inline double get_max_lat() const { return max_lat; }
    inline double get_max_lon() const { return max_lon; }
    inline const std::string& get_crs() const { return crs; }

    /** @brief Convert the bounding box to a string representation
     * @details The string representation is formatted for use in query parameters,
     *          excluding the "bbox=" prefix.
     */
    std::string to_string() const 
    {
      std::string result = to_query_string();
      std::string::size_type i = result.find( "&bbox=" );

      if( i != std::string::npos )
      {
          result.erase(i, 6);
      }
      return result;
    }

    /** @brief Convert the bounding box to a query string format
     */
    std::string to_query_string() const
    {
      std::stringstream stream;
      stream << std::fixed << std::setprecision( precision ) << min_lat;
      std::string min_lat_str = stream.str();
      stream.str( "" );
      stream << std::fixed << std::setprecision( precision ) << min_lon;
      std::string min_lon_str = stream.str();
      stream.str( "" );
      stream << std::fixed << std::setprecision( precision ) << max_lat;
      std::string max_lat_str = stream.str();
      stream.str( "" );
      stream << std::fixed << std::setprecision( precision ) << max_lon;
      std::string max_lon_str = stream.str();
      return crs.empty() ? "" : "&bbox=" + min_lat_str + "," + min_lon_str + "," +
              max_lat_str + "," + max_lon_str + "," + crs;
    }

  private:

    // Coordinates for the bounding box and the coordinate reference system (CRS)
    // The coordinates are in the order: min_lat, min_lon, max_lat, max_lon
    // CRS is a string representing the coordinate reference system (e.g., "EPSG:4326")
    // This class can be used to define a bounding box for spatial queries
    // or to specify the area of interest for map loading operations
    // The coordinates are typically in decimal degrees for geographic coordinates
    // or in meters for projected coordinates, depending on the CRS used
    double min_lat;
    double min_lon;
    double max_lat;
    double max_lon;
    std::string crs; // Coordinate Reference System
    constexpr static int precision = 6;
  };

  /** @brief Parameterized constructor for MapDownloader
   * @param server_url URL of the map server
   * @param username Username for authentication
   * @param password Password for authentication
   * @param project_name Name of the project on the map server
   * @param bounding_box Bounding box for the map data request
   * @param file_cache_path Path to the directory for caching map data
   * @param debug Boolean flag to enable or disable debug mode
   */
  MapDownloader( const std::string& server_url, const std::string& username, const std::string& password, 
      const std::string& project_name, const BoundingBox& bounding_box, const std::string& file_cache_path = "", 
      const bool curl_global_init = false, const bool curl_global_cleanup = false, 
      const bool debug_mode = false );

  /** @brief Downloads map data for a specific layer
   * @param layer_name Name of the layer to download
   * @return true if the download is successful, false otherwise
   */
  bool download_map( const std::string& layer_name );

  /** @brief Downloads map data for a specific layer within a bounding box
   * @param layer_name Name of the layer to download
   * @param bounding_box Bounding box for the map data request
   * @return true if the download is successful, false otherwise
   */
  bool download_map( const std::string& layer_name, const BoundingBox& bounding_box );

  /** @brief Gets the JSON data stored in the member variable json_data
   * @return Reference to the JSON data
   */
  inline nlohmann::json& get_json_data() { return json_data; };

  /** @brief Pretty prints the map data stored in json_data
   */
  void pretty_print_map();

  /** @brief Unloads the map data from memory 
   * @details Clears the read buffer of the internal curl wrapper and the internal JSON data
   */
  void unload_map();

  /** @brief Saves the map data stored in json_data to a file
   * @param filename The name of the file to save the map data to
   */
  void save_map( const std::string& filename );

  /** @brief Loads the map data from a file into member variable json_data
   * @param filename The name of the file to load the map data from
   */
  void load_map( const std::string& fileName );

  /** @brief Turns off the map cache
   * @details Disables caching of map data in the MapCache instance
   */
  void turn_off_cache();

  /** @brief Turns on the map cache
   * @details Enables caching of map data in the MapCache instance
   */
  void turn_on_cache();

  // Versions with more parameters for flexibility

   /** @brief Downloads map data for a specific layer within a bounding box
    * @param server_url URL of the map server
    * @param project_name Name of the project on the map server
    * @param layer_name Name of the layer to download
    * @param bounding_box Bounding box for the map data request
    * @return true if the download is successful, false otherwise
    */
  bool download_map( const std::string& server_url, const std::string& project_name, 
    const std::string& layer_name, const BoundingBox& bounding_box );

  /** @brief Unloads the map data from memory
   * @details Clears the internal read buffer of the curl wrapper and the given JSON data
   * @param json_data JSON object to be cleared
   */
  void unload_map( nlohmann::json& json_data );

  /** @brief Pretty prints the map data from a given JSON object
   * @param json_data JSON object to be pretty printed
   */
  static void pretty_print_map( const nlohmann::json& json_data );

  /** @brief Saves the map data from a given JSON object to a file
   * @param json_data JSON object containing the map data
   * @param filename Name of the file to save the map data to
   */
  static void save_map( const nlohmann::json& json_data, const std::string& filename );

  /** @brief Loads the map data from a file into a given JSON object
   * @param filename Name of the file to load the map data from
   * @param json_data JSON object to populate with the map data
   */
  static void load_map( const std::string& filename, nlohmann::json& json_data );
  
  // Getters for member variables

  inline const std::string& get_read_buffer() const { return curl_wrapper->get_read_buffer(); }
  inline const std::string& get_server_url() const { return server_url; }
  inline const BoundingBox& get_bounding_box() const { return bounding_box; }
  inline const std::string& get_project_name() const { return project_name; }
  inline const nlohmann::json& get_json_data() const { return json_data; }
  inline const MapCache& get_map_cache() const { return map_cache; }

private:

  /** @brief Downloads map data as JSON for a specific layer
   * @param layer_name Name of the layer to download
   * @return true if the download is successful, false otherwise
   */
  bool download_map_as_json( const std::string& layer_name );

  /** @brief Downloads map data as JSON for a specific layer within a bounding box
   * @param layer_name Name of the layer to download
   * @param bounding_box Bounding box for the map data request
   * @return true if the download is successful, false otherwise
   */
  bool download_map_as_json( const std::string& layer_name, const BoundingBox& bounding_box );

  /** @brief Downloads map data as JSON
   * @param server_url URL of the map server
   * @param project_name Name of the project on the map server
   * @param layer_name Name of the layer to download
   * @param bounding_box Bounding box for the map data request
   * @return CURLcode indicating the result of the operation
   */
  bool download_map_as_json( const std::string& server_url, const std::string& project_name, 
    const std::string& layer_name, const BoundingBox& bounding_box );

  /** @brief Parses JSON data from the internal read buffer and populates the internal JSON data object */
  void parse_json();

  /** @brief Parses JSON data from a string and populates the internal JSON data object */
  void parse_json( const std::string& json_str );

  /** @brief Parses JSON data from the internal read buffer and populates the provided JSON data object */
  void parse_json( nlohmann::json& json_data );

  /** @brief Parses JSON data from a string
   * @param json_str JSON string to be parsed
   * @param json_data JSON data object to be populated 
   */
  static void parse_json( const std::string& json_string, nlohmann::json& json_data );

  /** @brief Saves the map data stored in json_data to a file
   * @param filename Name of the file to save the map data to
   */
  void save_json( const std::string& filename );
 
  std::unique_ptr<CurlWrapper> curl_wrapper;
  const std::string server_url;
  const std::string project_name;
  const BoundingBox bounding_box;
  const bool debug_mode; // Flag to enable or disable debug mode
  nlohmann::json json_data;
  MapCache map_cache; // Instance of MapCache for caching map data
};
