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

#pragma once
#include <nlohmann/json.hpp>
#include <string>
#include "adore_map/bounding_box.hpp"

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
  const BoundingBox bbox;
  const std::string username;
  const std::string password;
  const std::string layer_name_reference_lines;
  const std::string layer_name_lane_borders;
  const std::string reference_line_filename;
  const std::string lane_border_filename;

  /** @brief Parameterized constructor that initializes the configuration from a JSON file
   * @param[in] filename Path to the JSON file
   */
  Config( const std::string& filename );

private:

  /** @brief Loads configuration from a JSON file and returns an nlohmann::json object
   * @details This function reads a JSON file and returns an nlohmann::json object 
   *          representing the key-value pairs of the configuration. It also prints out 
   *          the loaded configuration for verification.
   * @param[in] filename The path to the JSON file
   * @return An nlohmann::json object containing the loaded configuration
   */
  static nlohmann::json load_config( const std::string& filename );
};
