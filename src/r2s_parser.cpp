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

#include "adore_map/r2s_parser.h"

namespace adore
{
namespace r2s
{

// Helper function to parse LINESTRING and other fields

std::vector<std::string>
split_fields( const std::string& line )
{
  std::vector<std::string> fields;

  // Regular expression to match ID, LINESTRING, and other fields
  std::regex  re( R"((\d+),\"?LINESTRING \(([^)]+)\)\"?,(.*))" );
  std::smatch match;

  if( std::regex_match( line, match, re ) )
  {
    // Extract ID, LINESTRING, and remaining fields
    fields.push_back( match[1] ); // ID
    fields.push_back( match[2] ); // LINESTRING content (coordinates)

    // Process remaining fields manually by splitting on commas to handle empty fields
    std::string        remaining_fields = match[3];
    std::istringstream ss( remaining_fields );
    std::string        field;

    while( std::getline( ss, field, ',' ) )
    {
      field.erase( std::remove( field.begin(), field.end(), '"' ), field.end() ); // Remove quotes
      fields.push_back( field );
    }
  }
  else
  {
    std::cerr << "Unrecognized line format: " << line << std::endl;
  }
  return fields;
}

// Parse function for BorderDataR2SL with precise LINESTRING handling
BorderDataR2SL
parse_border_data_r2sl( const std::vector<std::string>& fields )
{
  BorderDataR2SL data;
  try
  {
    data.id = std::stoi( fields[0] );

    // Parse LINESTRING coordinates
    std::istringstream coords_stream( fields[1] );
    std::string        coord_pair;
    while( std::getline( coords_stream, coord_pair, ',' ) )
    {
      std::istringstream coord_stream( coord_pair );
      double             x, y;
      coord_stream >> x >> y;
      data.x.push_back( x );
      data.y.push_back( y );
    }

    // Parse remaining fields
    data.linetype                  = fields[fields.size() - 4];
    data.material                  = fields[fields.size() - 3];
    data.datasource_description_id = ( fields[fields.size() - 2] != "NULL" ) ? std::stoi( fields[fields.size() - 2] ) : 0;
    data.parent_id                 = ( fields[fields.size() - 1] != "NULL" ) ? std::stoi( fields[fields.size() - 1] ) : 0;
  }
  catch( const std::exception& e )
  {
    std::cerr << "Error parsing BorderDataR2SL: " << e.what() << std::endl;
  }
  return data;
}

// Similar parse function for BorderDataR2SR
BorderDataR2SR
parse_border_data_r2sr( const std::vector<std::string>& fields )
{
  BorderDataR2SR data;
  try
  {
    data.id = std::stoi( fields[0] );

    // Parse LINESTRING coordinates
    std::istringstream coords_stream( fields[1] );
    std::string        coord_pair;
    while( std::getline( coords_stream, coord_pair, ',' ) )
    {
      std::istringstream coord_stream( coord_pair );
      double             x, y;
      coord_stream >> x >> y;
      data.x.push_back( x );
      data.y.push_back( y );
    }

    // Parse remaining fields
    data.linetype                  = fields[fields.size() - 8];
    data.oneway                    = ( fields[fields.size() - 7] == "true" );
    data.category                  = fields[fields.size() - 6];
    data.turn                      = fields[fields.size() - 5];
    data.datasource_description_id = ( fields[fields.size() - 4] != "NULL" ) ? std::stoi( fields[fields.size() - 4] ) : 0;
    data.predecessor_id            = ( fields[fields.size() - 3] != "NULL" ) ? std::stoi( fields[fields.size() - 3] ) : 0;
    data.successor_id              = ( fields[fields.size() - 2] != "NULL" ) ? std::stoi( fields[fields.size() - 2] ) : 0;
    data.streetname                = fields[fields.size() - 1];
  }
  catch( const std::exception& e )
  {
    std::cerr << "Error parsing BorderDataR2SR: " << e.what() << std::endl;
  }
  return data;
}

// Load functions for R2SL and R2SR, handling line endings
std::vector<BorderDataR2SL>
load_border_data_from_r2sl_file( const std::string& file_name )
{

  std::string                 file_name_as_r2sl = file_name.substr( 0, file_name.size() - 1 ) + "l";
  std::vector<BorderDataR2SL> data_vector;
  std::ifstream               file( file_name_as_r2sl );
  if( !file )
  {
    std::cerr << "Failed to open file: " << file_name_as_r2sl << std::endl;
    return data_vector;
  }

  std::string line;
  std::getline( file, line ); // Skip header line

  while( std::getline( file, line ) )
  {
    if( !line.empty() && line.back() == '\r' )
    {
      line.pop_back(); // Remove carriage return for Windows-style line endings
    }
    auto fields = split_fields( line );
    if( fields.size() > 4 )
    {
      data_vector.push_back( parse_border_data_r2sl( fields ) );
    }
  }
  return data_vector;
}

std::vector<BorderDataR2SR>
load_border_data_from_r2sr_file( const std::string& file_name )
{
  std::vector<BorderDataR2SR> data_vector;
  std::ifstream               file( file_name );
  if( !file )
  {
    std::cerr << "Failed to open file: " << file_name << std::endl;
    return data_vector;
  }

  std::string line;
  std::getline( file, line ); // Skip header line

  while( std::getline( file, line ) )
  {
    if( !line.empty() && line.back() == '\r' )
    {
      line.pop_back(); // Remove carriage return for Windows-style line endings
    }
    auto fields = split_fields( line );
    if( fields.size() > 8 )
    {
      data_vector.push_back( parse_border_data_r2sr( fields ) );
    }
  }
  return data_vector;
}

// Load functions to load reference lines and lane borders from WFS URLs

// Load reference lines from a WFS layer using MapDownloader
std::vector<BorderDataR2SR> 
load_reference_lines( MapDownloader& downloader, const std::string& layer_name )
{
  std::vector<BorderDataR2SR> reference_lines;
  // Load reference lines from WFS
  if( !downloader.download_map( layer_name ) )
  { 
    std::cerr << "load_reference_lines: Failed to load map." << std::endl;
    return reference_lines;
  }
  nlohmann::json& json_data = downloader.get_json_data();
  if( json_data.contains( "features" ) && json_data[ "features" ].is_array() )
  {
    for( const auto& feature : json_data[ "features" ] )
    {
      if( feature.contains( "properties" ) && feature.contains( "geometry" ) )
      {
        BorderDataR2SR border;
        const auto& properties = feature[ "properties" ];
        if( properties[ "id" ].is_null() )
        {
          border.id = 0;
        } 
        else
        {
          border.id = properties.value( "id" , 0 );
        }
        if( properties[ "streetname" ].is_null() || properties[ "streetname" ].get<std::string>().empty() ) 
        {
          border.streetname = "NULL";
        }
        else
        {
          border.streetname = properties.value( "streetname", "NULL" );
        }
        if( properties[ "successor_id" ].is_null() )
        {
          border.successor_id = 0;
        }
        else
        {
          border.successor_id = properties.value( "successor_id", 0 );
        }
        if( properties[ "predecessor_id" ].is_null() )
        {
          border.predecessor_id = 0;
        }
        else
        {
          border.predecessor_id = properties.value( "predecessor_id", 0 );
        }
        if( properties[ "datasource_description_id" ].is_null() )
        {
          border.datasource_description_id = 0;
        }
        else
        {
          border.datasource_description_id = properties.value( "datasource_description_id", 0 );
        }
        if( properties[ "turn" ].is_null() || properties[ "turn" ].get<std::string>().empty() )
        {
          border.turn = "NULL";
        }
        else
        {
          border.turn = properties.value( "turn", "NULL" );
        }
        if( properties[ "category" ].is_null() || properties[ "category" ].get<std::string>().empty() )
        {
          border.category = "NULL";
        }
        else
        {
          border.category = properties.value( "category", "NULL" );
        }
        if( properties[ "oneway" ].is_null() || properties[ "oneway" ].get<std::string>().empty() )
        {
          border.oneway = false;
        }
        else 
        {
          std::istringstream( properties.value( "oneway", "false" ) ) >> std::boolalpha >> border.oneway;
        }
        if( properties[ "linetype" ].is_null() || properties[ "linetype" ].get<std::string>().empty() )
        {
          border.linetype = "NULL";
        }
        else
        {
          border.linetype = properties.value( "linetype", "NULL" );
        }
        if( feature[ "geometry" ].contains( "coordinates" ) && feature[ "geometry" ][ "coordinates" ].is_array() )
        {
          for(const auto& coord : feature[ "geometry" ][ "coordinates" ] )
          {
            if( coord.is_array() && coord.size() >= 2 )
            {
              border.x.push_back( round_to_six_decimal_places( coord[ 0 ].get<double>() ) );
              border.y.push_back( round_to_six_decimal_places( coord[ 1 ].get<double>() ) );
            }
            else
            {
              std::cerr << "load_reference_lines: Invalid coordinate format." << std::endl;
            }
          }
        }
        else
        {
          std::cerr << "load_reference_lines: Geometry does not contain valid coordinates." << std::endl;
        }
        reference_lines.push_back( border );
      }
      else
      {
        std::cerr << "load_reference_lines: Feature missing 'properties' or 'geometry'." << std::endl;
      }
    }
  }
  else
  {
    std::cerr << "load_reference_lines: Invalid JSON format: 'features' key not found or is not an array." << std::endl;
  }
   // Unload the map to free some of downloader's resources
  downloader.unload_map();
  return reference_lines;
}

// Load lane borders from a WFS layer using MapDownloader
std::vector<BorderDataR2SL> 
load_lane_borders( MapDownloader& downloader, const std::string& layer_name )
{
  std::vector<BorderDataR2SL> lane_borders;
  // Load lane borders from WFS
  if( !downloader.download_map( layer_name ) )
  { 
    std::cerr << "load_lane_borders: Failed to load map." << std::endl;
    return lane_borders;
  }
  nlohmann::json& json_data = downloader.get_json_data();
  if( json_data.contains( "features" ) && json_data[ "features" ].is_array() )
  {
    for( const auto& feature : json_data[ "features" ] )
    {
      if( feature.contains( "properties" ) && feature.contains( "geometry" ) )
      {
        BorderDataR2SL border;
        const auto& properties = feature[ "properties" ];
        if( properties[ "id" ].is_null() )
        {
          border.id = 0;
        }
        else
        {
          border.id = properties.value( "id", 0 );
        }
        if( properties[ "parent_id" ].is_null() )
        {
          border.parent_id = 0;
        }
        else
        {
          border.parent_id = properties.value( "parent_id" , 0);
        }
        if( properties[ "datasource_description_id" ].is_null() ) {
          border.datasource_description_id = 0;
        }
        else
        {
          border.datasource_description_id = properties.value( "datasource_description_id" , 0 );
        }
        if( properties[ "material" ].is_null() || properties[ "material" ].get<std::string>().empty() ) {
          border.material = "NULL";
        }
        else
        {
          border.material = properties.value( "material", "NULL" );
        }
        if( properties[ "type" ].is_null() || properties[ "type" ].get<std::string>().empty() )
        {
          border.linetype = "NULL";
        }
        else
        {
          border.linetype = properties.value( "type", "NULL" );
        }       
        if( feature[ "geometry" ].contains( "coordinates" ) && feature[ "geometry" ][ "coordinates" ].is_array() )
        {
          for( const auto& coord : feature[ "geometry" ][ "coordinates" ] )
          {
            if( coord.is_array() && coord.size() >= 2 )
            {
              border.x.push_back( round_to_six_decimal_places( coord[ 0 ].get<double>() ) );
              border.y.push_back( round_to_six_decimal_places( coord[ 1 ].get<double>() ) );
            }
            else
            {
              std::cerr << "load_lane_borders: nvalid coordinate format." << std::endl;
            }
          }
        }
        else
        {
          std::cerr << "load_lane_borders: Geometry does not contain valid coordinates." << std::endl;
        }
        lane_borders.push_back( border );
      }
      else
      {
        std::cerr << "load_lane_borders: Feature missing 'properties' or 'geometry'." << std::endl;
      }
    }
  }
  else
  {
    std::cerr << "load_lane_borders: Invalid JSON format: 'features' key not found or is not an array." << std::endl;
  }
   // Unload the map to free some of downloader's resources
  downloader.unload_map();
  return lane_borders;
}

// Function to compare two vectors of doubles for closeness within a tolerance
bool are_close(const std::vector<double>& a, const std::vector<double>& b, double tolerance)
{
  if( a.size() != b.size() )
  {
    return false;
  }
  for( size_t i = 0; i < a.size(); ++i )
  {
    if( std::abs( a[i] - b[i] ) > tolerance )
    {
      return false;
    }
  }
  return true;
}

} // namespace r2s
} // namespace adore
