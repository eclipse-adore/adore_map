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

#include "adore_map/config.hpp"
#include "adore_map/bounding_box.hpp"
#include "adore_map/json_file_helpers.hpp"

Config::Config( const std::string& filename ) : 
  props( load_config( filename ) ),
  server_url( props[ "url" ] ),
  project_name( props[ "project_name" ] ),
  target_srs( props[ "target_srs" ] ),
  bbox( BoundingBox( props[ "bbox" ], target_srs ) ),
  username( props[ "username" ] ),
  password( props[ "password" ] ),
  layer_name_reference_lines( props[ "reference_lines" ] ),
  layer_name_lane_borders( props[ "laneborders" ] ),
  reference_line_filename( props[ "output" ].get<std::string>() + ".rjson" ),
  lane_border_filename( props[ "output" ].get<std::string>() + ".ljson" )
{
}

// Static method to load configuration from a JSON file and returns an nlohmann::json object
nlohmann::json 
Config::load_config( const std::string& filename )
{
  nlohmann::json config;

  JsonFileHelpers::load( filename, config, "Config::load_config" );
  std::cout << "Config::load_config: Loaded configuration from " << filename << ":" << std::endl;
  for( auto& el : config.items() )
  {
    std::cout << el.key() << " = " << el.value() << std::endl;
  }
  return config;
}
