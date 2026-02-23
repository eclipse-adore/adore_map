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
#include <string>
#include <vector>

/**
 * @brief Class representing a bounding box for spatial queries
 */
class BoundingBox
{
public:
 
  /** @brief Parameterized constructor for BoundingBox: takes min and max latitude and longitude, and a coordinate reference system (CRS)
   * @param[in] min_lat Minimum latitude of the bounding box
   * @param[in] min_lon Minimum longitude of the bounding box
   * @param[in] max_lat Maximum latitude of the bounding box
   * @param[in] max_lon Maximum longitude of the bounding box
   * @param[in] crs Coordinate reference system (e.g., "EPSG:4326")
   * @details The constructor performs a sanity check on the provided coordinates to ensure they form a valid bounding box.
   */
  BoundingBox( const double min_lat, const double min_lon, const double max_lat, const double max_lon, const std::string& crs );

  /** @brief Parameterized constructor for BoundingBox: takes a vector of coordinates and a coordinate reference system (CRS)
   * @param[in] coords A vector of doubles representing the bounding box coordinates in the order: min_lat, min_lon, max_lat, max_lon
   * @param[in] crs Coordinate reference system (e.g., "EPSG:4326")
   * @details The constructor performs a sanity check on the provided coordinates to ensure they form a valid bounding box.
   */
  BoundingBox( const std::vector<double>& coords, const std::string& crs );
    
  // Getters for the bounding box coordinates and CRS
  inline const double get_min_lat() const { return min_lat; }
  inline const double get_min_lon() const { return min_lon; }
  inline const double get_max_lat() const { return max_lat; }
  inline const double get_max_lon() const { return max_lon; }
  inline const std::string& get_crs() const { return crs; }

  /** @brief Convert the bounding box to a string representation
   * @details The string representation is formatted for use in query parameters,
   *          excluding the "bbox=" prefix.
   */
  std::string to_string() const; 

  /** @brief Convert the bounding box to a query string format
   */
  std::string to_query_string() const;

private:

 /** @brief Checks a bounding box given as four coordinates for sanity and returns a BoundingBox object
   * @details First coordinate pair for lower left corner, second coordinate pair for upper right corner
   * @details Validates the provided coordinates and constructs a BoundingBox object
   * @note The \ref target_srs parameter is used to set the coordinate reference system of the bounding box
   * @param[in] coords A vector of doubles representing the bounding box coordinates
   * @param[in] target_srs The target spatial reference system
   * @return A BoundingBox object representing the parsed bounding box
   */
  static void sanity_check_bounding_box( const std::vector<double>& coords, 
    const std::string& target_srs );

  // Coordinates for the bounding box and the coordinate reference system (CRS)
  // The coordinates are in the order: min_lat, min_lon, max_lat, max_lon
  // CRS is a string representing the coordinate reference system (e.g., "EPSG:4326")
  // This class can be used to define a bounding box for spatial queries
  // or to specify the area of interest for map loading operations
  // The coordinates are typically in decimal degrees for geographic coordinates
  // or in meters for projected coordinates, depending on the CRS used
  const double min_lat;
  const double min_lon;
  const double max_lat;
  const double max_lon;
  const std::string crs; // Coordinate Reference System
  constexpr static int precision = 6;
};
