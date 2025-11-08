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
#include <PropertiesParser.h>
#include "adore_map/map_downloader.hpp"
using namespace cppproperties;

/**
 * @brief Config class to handle configuration properties for the map downloader
 */
class Config
{
public:

  const cppproperties::Properties props;
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

  /** @brief Constructor that initializes the configuration from a properties file
   * @param filename Path to the properties file
   */
  Config( const std::string& filename )
    : props( load_properties( filename ) ),
    server_url( props.GetProperty( "url" ) ),
    project_name( props.GetProperty( "project_name" ) ),
    target_srs(props.GetProperty( "target_srs" ) ),
    bbox( parse_bounding_box( props.GetProperty( "bbox" ), target_srs ) ),
    username( props.GetProperty( "username" ) ),
    password( props.GetProperty( "password" ) ),
    layer_name_reference_lines( props.GetProperty( "reference_lines" ) ),
    layer_name_lane_borders( props.GetProperty( "laneborders" ) ),
    reference_line_filename( props.GetProperty( "output" ) + ".rs2r" ),
    lane_border_filename( props.GetProperty( "output" ) + ".r2sl" )
  {
  }

private:

  /** @brief Loads properties from a file and returns a Properties object
   * @details This function reads a properties file using the PropertiesParser and
   *          returns a Properties object containing the key-value pairs. It also
   *          prints out the loaded properties for verification.
   * @param filename The path to the properties file
   * @return A Properties object containing the loaded properties
   */
  static Properties load_properties( const std::string& filename );
  
  /** @brief Parses a bounding box string and returns a BoundingBox object
   * @details The bounding box string should be in the format "min_lat,min_lon,max_lat,max_lon".
   * @note If the string is empty, a default BoundingBox with all zeros is returned.
   * @note The target_srs parameter is used to set the coordinate reference system of the bounding box.
   * @note The function splits the string by commas and converts the values to double
   * @param bbox_str The bounding box string in the format "min_lat,min_lon,max_lat,max_lon"
   * @param target_srs The target spatial reference system
   * @return A BoundingBox object representing the parsed bounding box
   */
  static MapDownloader::BoundingBox parse_bounding_box( const std::string& bboxStr, const std::string& targetSrs );
};
