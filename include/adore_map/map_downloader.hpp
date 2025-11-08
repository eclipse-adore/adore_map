
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

class MapCache; // Forward declaration to avoid circular dependency

/**
 * @brief Class for downloading map data from a server with caching capabilities
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
    BoundingBox() : min_lat( 0.0 ), min_lon( 0.0 ), max_lat( 0.0 ), max_lon( 0.0 ), crs( "" ) {}
    BoundingBox( double min_lat, double min_lon, double max_lat, double max_lon, const std::string& crs )
        : min_lat( min_lat ), min_lon( min_lon ), max_lat( max_lat ), max_lon( max_lon ), crs( crs ) {}
        
    // Getters for the bounding box coordinates and CRS

    inline double get_min_lat() const { return min_lat; }
    inline double get_min_lon() const { return min_lon; }
    inline double get_max_lat() const { return max_lat; }
    inline double get_max_lon() const { return max_lon; }
    inline std::string get_crs() const { return crs; }

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

  /** @brief Default constructor for MapDownloader
   */
  MapDownloader();

  /**
   * @brief Parameterized constructor for MapDownloader
   * @param server_url URL of the map server
   * @param username Username for authentication
   * @param password Password for authentication
   * @param project_name Name of the project on the map server
   * @param bounding_box Bounding box for the map data request
   * @param file_cache_path Path to the directory for caching map data
   * @param debug Boolean flag to enable or disable debug mode
   */
  MapDownloader( const std::string& server_url, const std::string& username, const std::string& password, 
      const std::string& project_name, const BoundingBox& bounding_box = BoundingBox( 0.0, 0.0, 0.0, 0.0, "" ), 
      std::string file_cache_path = "", const bool debug_mode = false );

  /** @brief Destructor for MapDownloader
   */
  ~MapDownloader();

  // Methods to initialize and clean up cURL, load map data as JSON, unload map data, parse JSON, 
  // pretty print JSON, and create JSON files
  // Versions with less parameters using member variables.

  /** @brief Initializes the MapDownloader instance by setting up cURL
   * @return true if initialization is successful, false otherwise
   */
  bool initialize();

  /** @brief Cleans up the MapDownloader instance by releasing cURL resources
   */
  void cleanup();

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
   * @details Clears the internal read buffer and the internal JSON data
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

  /** @brief Static method to initialize cURL
   * @details This method initializes the cURL library, sets options for the request,
   *          and prepares the cURL handle for use.
   * @param username Username for authentication
   * @param password Password for authentication
   * @param curl Pointer to a cURL handle
   * @param read_buffer Pointer to a string where the response will be stored
   * @return CURLcode indicating the result of the initialization
   */
  static CURLcode initialize_curl( const std::string& username, const std::string& password, CURL **curl, 
    std::string* read_buffer );

  /** @brief Static method to clean up cURL resources
   * @param curl Pointer to the cURL handle to be cleaned up
   */
  static void curl_cleanup( CURL *curl );

  /** @brief Downloads map data as JSON using cURL
   * @param curl Pointer to the cURL handle
   * @param server_url URL of the map server
   * @param project_name Name of the project on the map server
   * @param layer_name Name of the layer to download
   * @param bounding_box Bounding box for the map data request
   * @param read_buffer Pointer to the string where the response will be stored
   * @return CURLcode indicating the result of the operation
   */
  CURLcode download_map( CURL *curl, const std::string& server_url, const std::string& project_name, 
    const std::string& layer_name, const BoundingBox& bounding_box, std::string* read_buffer );

  /** @brief Unloads the map data from memory 
   * @details Clears the given read buffer and JSON data
   * @param read_buffer Pointer to the read buffer string
   * @param json_data Reference to the JSON data object
   */
  static void unload_map( std::string* read_buffer, nlohmann::json& json_data );

  /** @brief Pretty prints the map data from a given JSON object
   * @param json_data The JSON object to be pretty printed
   */
  static void pretty_print_map( const nlohmann::json& json_data );

  /** @brief Saves the map data from a given JSON object to a file
   * @param json_data The JSON object containing the map data
   * @param filename The name of the file to save the map data to
   */
  static void save_map( const nlohmann::json& json_data, const std::string& filename );

  /** @brief Saves the map data from a JSON string to a file
   * @param json_str The JSON string containing the map data
   * @param filename The name of the file to save the map data to
   */
  static void save_map( const std::string& json_str, const std::string& filename );

  /** @brief Loads the map data from a file into a given JSON object
   * @param filename The name of the file to load the map data from
   * @param json_data Reference to the JSON object to populate with the map data
   */
  static void load_map( const std::string& filename, nlohmann::json& json_data );
  
  // Getters for member variables

  //inline std::string get_read_buffer() const { return read_buffer; }
  inline const std::string& get_read_buffer() const { return read_buffer; }
  inline const std::string& get_server_url() const { return server_url; }
  inline const BoundingBox& get_bounding_box() const { return bounding_box; }
  inline const std::string& get_username() const { return username; }
  inline const std::string& get_password() const { return password; }
  inline const std::string& get_project_name() const { return project_name; }
  inline const nlohmann::json& get_json_data() const { return json_data; }
  inline const std::string& get_file_cache_path() const { return file_cache_path; }
  inline MapCache* get_map_cache() const { return map_cache; }

private:
  /** @brief cURL write callback function
   * @details This function is called by cURL when data is received from the server. 
   * @param ptr Pointer to the received data
   * @param size Size of each data element
   * @param nmemb Number of data elements
   * @param userdata Pointer to the user data (in this case, a std::string)
   * @return Number of bytes written
   */
  static size_t write_callback( char* ptr, size_t size, size_t nmemb, void* userdata );

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

  /** @brief Downloads map data as JSON using cURL
   * @param curl Pointer to the cURL handle
   * @param server_url URL of the map server
   * @param project_name Name of the project on the map server
   * @param layer_name Name of the layer to download
   * @param bounding_box Bounding box for the map data request
   * @param read_buffer Pointer to the string where the response will be stored
   * @return CURLcode indicating the result of the operation
   */
  CURLcode download_map_as_json( CURL *curl, const std::string& server_url, const std::string& project_name, 
    const std::string& layer_name, const BoundingBox& bounding_box, std::string* read_buffer );

  /** @brief Parses JSON data from a string
   * @param json_str The JSON string to be parsed
   * @param json_data Reference to the JSON data object to be populated 
   */
  static void parse_json( const std::string& json_string, nlohmann::json& json_data );

  /** @brief Saves the map data stored in json_data to a file
   * @param filename The name of the file to save the map data to
   */
  void save_json( const std::string& filename );

  /** @brief Saves the map data from a given JSON object to a file
   * @param json_data The JSON object containing the map data
   * @param filename The name of the file to save the map data to
   */
  static void save_json( const nlohmann::json& json_data, const std::string& filename );
  
  CURL* curl;
  std::string read_buffer;
  std::string server_url;
  std::string username;
  std::string password;
  std::string project_name;
  BoundingBox bounding_box;
  std::string file_cache_path; // Path to the cache directory for saving map data
  bool debug_mode; // Flag to enable or disable debug mode
  nlohmann::json json_data;
  MapCache* map_cache; // Instance of MapCache for caching map data
};