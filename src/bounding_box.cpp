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

#include <sstream>
#include <iomanip>
#include <iostream>
#include "adore_map/bounding_box.hpp"


// Parameterized constructor for BoundingBox: takes min and max latitude and longitude, and a coordinate reference system (CRS)
// The constructor performs a sanity check on the provided coordinates to ensure they form a valid bounding box
BoundingBox::BoundingBox( const double min_lat, const double min_lon, const double max_lat, const double max_lon, const std::string& crs )
  : min_lat( min_lat ), min_lon( min_lon ), max_lat( max_lat ), max_lon( max_lon ), crs( crs )
{
  std::vector<double> coords = { min_lat, min_lon, max_lat, max_lon };
  sanity_check_bounding_box( coords, crs );
}

// Parameterized constructor for BoundingBox: takes a vector of coordinates and a coordinate reference system (CRS)
// The constructor performs a sanity check on the provided coordinates to ensure they form a valid bounding box
BoundingBox::BoundingBox( const std::vector<double>& coords, const std::string& crs )
  : min_lat( coords[0] ), min_lon( coords[1] ), max_lat( coords[2] ), max_lon( coords[3] ), crs( crs )
{
  sanity_check_bounding_box( coords, crs );
}
  
// Convert the bounding box to a string representation
// The string representation is formatted for use in query parameters, excluding the "bbox=" prefix
std::string 
BoundingBox::to_string() const 
{
  std::string result = to_query_string();
  std::string::size_type i = result.find( "&bbox=" );

  if( i != std::string::npos )
  {
      result.erase(i, 6);
  }
  return result;
}

// Convert the bounding box to a query string format
std::string 
BoundingBox::to_query_string() const
{
  std::stringstream stream;
  stream << std::fixed << std::setprecision( precision ) << min_lat;
  const std::string min_lat_str = stream.str();
  stream.str( "" );
  stream << std::fixed << std::setprecision( precision ) << min_lon;
  const std::string min_lon_str = stream.str();
  stream.str( "" );
  stream << std::fixed << std::setprecision( precision ) << max_lat;
  const std::string max_lat_str = stream.str();
  stream.str( "" );
  stream << std::fixed << std::setprecision( precision ) << max_lon;
  const std::string max_lon_str = stream.str();
  return "&bbox=" + min_lat_str + "," + min_lon_str + "," +
          max_lat_str + "," + max_lon_str + "," + crs;
}

// Static method to check a bounding box given as four coordinates for sanity and returns a BoundingBox object
// First coordinate pair for lower left corner, second coordinate pair for upper right corner
// Validates the provided coordinates and constructs a BoundingBox object
// The target_srs parameter is used to set the coordinate reference system of the bounding box
void 
BoundingBox::sanity_check_bounding_box( const std::vector<double>& coords, 
  const std::string& target_srs ) 
{
  if( coords.empty() )
  {
    std::cerr << "BoundingBox::sanity_check_bounding_box: Error: Empty coordinate vector." << std::endl;
    throw std::invalid_argument( "Empty coordinate vector." );
  }
  if( coords.size() != 4 )
  {
    std::cerr << "BoundingBox::sanity_check_bounding_box: Error: Invalid coordinate vector. "
      << "Expected 4 values, got " << coords.size() << "." << std::endl;
    throw std::invalid_argument( "Invalid coordinate vector." );
  }
  if( coords[0] >= coords[2] || coords[1] >= coords[3] )
  {
    std::cerr << "BoundingBox::sanity_check_bounding_box: Error: Invalid bounding box coordinates. "
      << "Minimum coordinates must be less than maximum coordinates." << std::endl;
    throw std::invalid_argument( "Invalid bounding box coordinates." );
  }
  if( target_srs.empty() )
  {
    std::cerr << "BoundingBox::sanity_check_bounding_box: Error: Empty target SRS." << std::endl;
    throw std::invalid_argument( "Empty target SRS." );
  }
}
