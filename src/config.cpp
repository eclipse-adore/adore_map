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
#include <sstream>
#include "adore_map/config.hpp"

// Checks a bounding box given as four coordinates for sanity and returns a BoundingBox object
const MapDownloader::BoundingBox Config::sanity_check_bounding_box( const std::vector<double>& coords, 
  const std::string& target_srs )
{  
  if( coords.empty() )
  {
    std::cerr << "Config::sanity_check_bounding_box: Error: Empty coordinate vector." << std::endl;
    throw std::invalid_argument( "Empty coordinate vector." );
  }
  if( coords.size() != 4 )
  {
    std::cerr << "Config::sanity_check_bounding_box: Error: Invalid coordinate vector. "
      << "Expected 4 values, got " << coords.size() << "." << std::endl;
    throw std::invalid_argument( "Invalid coordinate vector." );
  }
  if( coords[0] >= coords[2] || coords[1] >= coords[3] )
  {
    std::cerr << "Config::sanity_check_bounding_box: Error: Invalid bounding box coordinates. "
      << "Minimum coordinates must be less than maximum coordinates." << std::endl;
    throw std::invalid_argument( "Invalid bounding box coordinates." );
  }
  return MapDownloader::BoundingBox
    (
      coords[0], coords[1], coords[2], coords[3], 
      target_srs
    );
}

// Loads configuration from a JSON file and returns an nlohmann::json object
nlohmann::json Config::load_config( const std::string& filename )
{
  nlohmann::json config;

  std::ifstream file( filename );
  if( file.is_open() )
  {
    try
    {
      file >> config;
    } 
    catch ( const nlohmann::json::parse_error& e ) 
    {
      std::cerr << "Config::load_config: JSON parse error in file " << filename 
        << ": " << e.what() << std::endl;
      file.close();
      throw std::runtime_error( "JSON parse error in file: " + filename );
    }
    file.close();
    std::cout << "Config::load_config: Loaded configuration from " << filename << ":" << std::endl;
    for( auto& el : config.items() )
    {
      std::cout << el.key() << " = " << el.value() << std::endl;
    }
    return config; 
  }
  else 
  {
    std::cerr << "Config::load_config: Failed to open JSON file: " << filename << std::endl;
    throw std::runtime_error( "Failed to open JSON file: " + filename );
  }
}
