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
#include "adore_map/map_downloader.hpp"
#include <nlohmann/json.hpp>

/**
 * @brief Class to handle the configuration for the map downloader, stored in a JSON file 
 */
class Config
{
public:

  const nlohmann::json props;
  const std::string server_url;
  const std::string project_name;
  const std::string target_srs;
  const MapDownloader::BoundingBox bbox;
  const std::string username;
  const std::string password;
  const std::string layer_name_reference_lines;
  const std::string layer_name_lane_borders;
  const std::string reference_line_filename;
  const std::string lane_border_filename;

  /** @brief Constructor that initializes the configuration from a JSON file
   * @param filename Path to the JSON file
   */
  Config( const std::string& filename ) : 
    props( load_config( filename ) ),
    server_url( props[ "url" ] ),
    project_name( props[ "project_name" ] ),
    target_srs( props[ "target_srs" ] ),
    bbox( sanity_check_bounding_box( props[ "bbox" ], target_srs ) ),
    username( props[ "username" ] ),
    password( props[ "password" ] ),
    layer_name_reference_lines( props[ "reference_lines" ] ),
    layer_name_lane_borders( props[ "laneborders" ] ),
    reference_line_filename( props[ "output" ].get<std::string>() + ".rs2r" ),
    lane_border_filename( props[ "output" ].get<std::string>() + ".r2sl" )
  {
  }

private:

  /** @brief Loads configuration from a JSON file and returns an nlohmann::json object
   * @details This function reads a JSON file and returns an nlohmann::json object 
   *          representing the key-value pairs of the configuration. It also prints out 
   *          the loaded configuration for verification.
   * @param filename The path to the JSON file
   * @return An nlohmann::json object containing the loaded configuration
   */
  static nlohmann::json load_config( const std::string& filename );
  
  /** @brief Checks a bounding box given as four coordinates for sanity and returns a BoundingBox object
   * @details First coordinate pair for lower left corner, second coordinate pair for upper right corner
   * @details Validates the provided coordinates and constructs a BoundingBox object
   * @note The \ref target_srs parameter is used to set the coordinate reference system of the bounding box
   * @param coords A vector of doubles representing the bounding box coordinates
   * @param target_srs The target spatial reference system
   * @return A BoundingBox object representing the parsed bounding box
   */
  static const MapDownloader::BoundingBox sanity_check_bounding_box( const std::vector<double>& coords, 
    const std::string& target_srs );
};
