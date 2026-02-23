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

#include <string>

#include "adore_map/map.hpp"
#include "adore_map/r2s_parser.h"
#include "adore_map/map_downloader.hpp"

#include "OpenDriveMap.h"
#include "RoutingGraph.h"

namespace adore
{
namespace map
{

// Struct to hold a border and its lateral offset for sorting
struct BorderWithOffset
{
  Border clipped_border;
  double lateral_offset;
};

using Bordermap = std::unordered_map<int, std::vector<std::shared_ptr<r2s::BorderDataR2SL>>>;

class MapLoader
{
public:

  // Static method to load a Map from an R2S file
  static Map load_from_r2s_file( const std::string& map_file_location, bool allow_lane_changes = true, bool ignore_non_driving = false );

  static Map load_from_xodr_file( const std::string& map_file_location, bool ignore_non_driving = false );

  static Map load_from_file( const std::string& map_file_location, bool allow_lane_changes = true, bool ignore_non_driving = false );

  /** @brief Downloads map data from a WFS server and constructs a Map object
   * @param[in] downloader MapDownloader instance to use for downloading map data
   * @param[in] reference_lines_layer_name Name of the layer containing reference lines
   * @param[in] lane_borders_layer_name Name of the layer containing lane borders
   * @param[in] allow_lane_changes Boolean flag to indicate whether lane changes should be allowed in the resulting Map
   * @param[in] ignore_non_driving Boolean flag to indicate whether non-driving lanes should be ignored in the resulting Map
   * @return A Map object constructed from the downloaded map data
   */
  static Map download_from_wfs( MapDownloader& downloader, const std::string& reference_lines_layer_name, 
    const std::string& lane_borders_layer_name, bool allow_lane_changes = true, bool ignore_non_driving = false );

private:

  static void create_from_r2s( Map& map, const std::vector<r2s::BorderDataR2SR>& standard_lines,
                               const std::vector<r2s::BorderDataR2SL>& lane_boundaries, bool allow_lane_changes );

  static Border create_reference_line( const adore::r2s::BorderDataR2SR& r2s_ref_line );

  static std::vector<Border> process_relevant_borders( adore::map::Bordermap&            refline_to_border,
                                                       const adore::r2s::BorderDataR2SR& r2s_ref_line, adore::map::Border& reference_line );

  static std::vector<double> collect_s_positions( Border& reference_line, const std::vector<Border>& borders );

  static std::vector<BorderWithOffset> get_clipped_borders( const std::vector<Border>& borders, const Border& ref_line_clipped,
                                                            double s_start, double s_end );

  static void make_lane( const BorderWithOffset& inner_border, const BorderWithOffset& outer_border, Road& road, Map& map,
                         const std::unordered_map<int, std::shared_ptr<r2s::BorderDataR2SL>>& id_to_border );

  static double compute_lateral_offset( const Border& reference_line, const MapPoint& target_point );

  static void set_quadtree_bounds( Map& map, const std::vector<adore::r2s::BorderDataR2SR>& standard_lines,
                                   const std::vector<adore::r2s::BorderDataR2SL>& lane_boundaries );

  static void set_quadtree_bounds( Map& map, const odr::OpenDriveMap& xodr_map );

  static size_t generate_lane_id();

  // Constants
  constexpr static double LANE_CONNECTION_DIST = 1.2;
  constexpr static double MIN_LANE_LENGTH      = 0.5;

  static RoadGraph                         infer_graph_from_proximity_of_lanes( Map& map, double proximity );
  static void                              add_parallel_connections_same_road( Map& map, RoadGraph& graph, double lane_change_penalty );
  static std::pair<double, ConnectionType> calculate_lane_distance( const Lane& from_lane, const Lane& to_lane );

  static std::pair<Border, Border> xodr_mesh_to_borders( const odr::Mesh3D& mesh, size_t lane_id, double s0 );
};

} // namespace map
} // namespace adore
