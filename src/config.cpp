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

// Parses a bounding box string and returns a BoundingBox object
MapDownloader::BoundingBox Config::parse_bounding_box( const std::string& bbox_str, const std::string& target_srs )
{  
  if( bbox_str.empty() )
  {
    return MapDownloader::BoundingBox( 0.0, 0.0, 0.0, 0.0, target_srs );
  }
  std::vector<double> vect;
  std::stringstream ss( bbox_str );
  for( double i; ss >> i; )
  {
    vect.push_back( i );
    if( ss.peek() == ',' || ss.peek() == ' ' )
    {
      ss.ignore();
    }
  }
  return MapDownloader::BoundingBox
    (
      vect.size() >= 1 ? vect[0] : 0.0,
      vect.size() >= 2 ? vect[1] : 0.0,
      vect.size() >= 3 ? vect[2] : 0.0,
      vect.size() >= 4 ? vect[3] : 0.0,
      target_srs
    );
}

// Loads properties from a file and returns a Properties object
Properties Config::load_properties( const std::string& filename )
{
    Properties props = PropertiesParser::Read( filename );
    auto names = props.GetPropertyNames();
    if( names.empty() )
    {
      std::cerr << "Config::load_properties: Warning: No properties found in file: " << filename << std::endl;
    }
    for( const auto& name : names )
    {
      std::cout << name << " = " << props.GetProperty( name ) << std::endl;
    }
    return props;
} 
