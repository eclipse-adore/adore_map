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
#include <nlohmann/json.hpp>
#include <iostream>
#include <string>
#include <fstream>

/**
 * @brief Helper class for saving and loading JSON files. If a context string is provided,
 *        it is used in error messages to indicate the calling context.
 */
class JsonFileHelpers
{
public:

  /** @brief Saves the given JSON data to a file
   * @param json_data The JSON data to save
   * @param filename The name of the file to save the JSON data to
   */
  static void save( const nlohmann::json& json_data, const std::string& filename, 
    const std::string& context = "" )
  {
    std::ofstream file(filename);
    if( file.is_open() )
    {
      try 
      {
        file << json_data;
      }
      catch( const std::exception& e )
      {
        std::cerr << ( context.empty() ? "JsonFileHelpers::save" : context ) 
          << ": Error writing JSON data to file " << filename << ": " << e.what() << std::endl;
        throw std::runtime_error( "Error writing JSON data to file " + filename + ": " + e.what() );
      }   
    }
    else
    {
      std::cerr << ( context.empty() ? "JsonFileHelpers::save" : context ) 
        << ": Failed to open JSON file for writing: " << filename << std::endl;
      throw std::runtime_error( "Failed to open JSON file for writing: " + filename );
    }
  }

  /** @brief Loads JSON data from a file
   * @param filename The name of the file to load the JSON data from
   * @param json_data The JSON object to load the data into
   */
  static void load( const std::string& filename, nlohmann::json& json_data, 
    const std::string& context = "" )
  {
    std::ifstream file( filename );
    if( file.is_open() )
    {
      try 
      {
        json_data = nlohmann::json::parse( file );
      }
      catch( const nlohmann::json::parse_error& e )
      {
        std::cerr << ( context.empty() ? "JsonFileHelpers::load" : context ) << ": JSON parse error in file " 
          << filename << ": " << e.what() << std::endl;
        throw std::runtime_error( "JSON parse error in file " + filename + ": " + e.what() ) ;
      }
    }  
    else 
    {
      std::cerr << ( context.empty() ? "JsonFileHelpers::load" : context ) << ": Failed to open JSON file: " << filename << std::endl;
      throw std::runtime_error( "Failed to open JSON file: " + filename );
    }
  }
};
