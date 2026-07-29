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

#include "adore_map/route.hpp"

namespace adore
{
namespace map
{

void
Route::add_route_section( Border& lane_to_add, const MapPoint& start_point, const MapPoint& end_point, bool reverse /* = false */ )
{
  if( lane_to_add.interpolated_points.empty() )
    return;

  auto next     = std::make_shared<RouteSection>();
  next->lane_id = lane_to_add.interpolated_points[0].parent_id;

  // 1) default lane-local interval: full lane, with direction
  if( reverse )
  {
    next->end_s   = lane_to_add.interpolated_points.front().s;
    next->start_s = lane_to_add.interpolated_points.back().s;
  }
  else
  {
    next->start_s = lane_to_add.interpolated_points.front().s;
    next->end_s   = lane_to_add.interpolated_points.back().s;
  }

  // 2) crop by actual route start / end if this lane contains them
  if( start_point.parent_id == next->lane_id )
    next->start_s = start_point.s;
  if( end_point.parent_id == next->lane_id )
    next->end_s = end_point.s;

  // 3) lane-change case: if previous section exists and we’re switching lanes
  //    via a PARALLEL connection, align the new lane's start_s with the
  //    previous lane's "exit s" so we don't go back to s=0 of the lane.
  if( !sections.empty() && map )
  {
    const auto& prev = sections.back();
    if( prev && prev->lane_id != next->lane_id )
    {
      auto conn_opt = map->lane_graph.find_connection( prev->lane_id, next->lane_id );
      if( conn_opt && conn_opt->connection_type == PARALLEL )
      {
        // We leave the previous lane at prev->end_s (regardless of direction),
        // so start the new lane at the same longitudinal s along the road.
        next->start_s = prev->end_s;
      }
    }
  }

  lane_to_sections[next->lane_id] = next;
  sections.push_back( next );
}

double
Route::get_length() const
{
  if( reference_line.empty() )
  {
    return 0.0;
  }
  auto reference_line_iter = reference_line.end();
  reference_line_iter--;
  return reference_line_iter->first;
}

std::deque<MapPoint>
Route::get_shortened_route( double start_s, double desired_length ) const
{
  std::deque<MapPoint> result;

  auto reference_line_iter = reference_line.lower_bound( start_s );
  auto upper_bound         = reference_line.upper_bound( start_s + desired_length );

  while( reference_line_iter != reference_line.end() && reference_line_iter != upper_bound )
  {
    result.push_back( reference_line_iter->second );
    reference_line_iter++;
  }
  return result;
}

MapPoint
Route::get_map_point_at_s( double distance ) const
{
  return interpolate_at_s<MapPoint>( distance );
}

math::Pose2d
Route::get_pose_at_s( double distance ) const
{
  return interpolate_at_s<math::Pose2d>( distance );
}

double
Route::get_curvature_at_s( double s ) const
{
  const double delta_s     = 0.5;
  auto         pose_before = get_pose_at_s( s - delta_s );
  auto         pose_after  = get_pose_at_s( s + delta_s );

  double delta_yaw = math::normalize_angle( pose_after.yaw - pose_before.yaw );
  double curvature = delta_yaw / ( 2.0 * delta_s );

  return curvature;
}

void
Route::initialize_reference_line()
{
  reference_line.clear();
  if( !map )
  {
    return;
  }

  double route_s_accum = 0.0;

  for( auto& section : sections )
  {
    // 1) find lane
    auto lane_it = map->lanes.find( section->lane_id );
    if( lane_it == map->lanes.end() )
      continue;

    const auto& lane    = *lane_it->second;
    const auto& cpoints = lane.borders.center.interpolated_points;
    if( cpoints.empty() )
      continue;

    // 2) lane-local interval and direction
    const bool   reverse     = ( section->end_s < section->start_s );
    const double lane_s_min  = std::min( section->start_s, section->end_s );
    const double lane_s_max  = std::max( section->start_s, section->end_s );
    const double section_len = lane_s_max - lane_s_min;

    if( section_len <= 0.0 )
      continue;

    // global start s for this section
    section->route_s = route_s_accum;

    // 3) iterate over lane center points in correct direction,
    //    but only within [lane_s_min, lane_s_max]
    if( !reverse )
    {
      for( const auto& pt : cpoints )
      {
        if( pt.s < lane_s_min || pt.s > lane_s_max )
          continue;

        const double local_s  = pt.s - lane_s_min; // 0 .. section_len
        const double ref_s    = route_s_accum + local_s;
        reference_line[ref_s] = pt;
      }
    }
    else
    {
      for( auto it = cpoints.rbegin(); it != cpoints.rend(); ++it )
      {
        const auto& pt = *it;
        if( pt.s < lane_s_min || pt.s > lane_s_max )
          continue;

        const double local_s  = lane_s_max - pt.s; // 0 .. section_len
        const double ref_s    = route_s_accum + local_s;
        reference_line[ref_s] = pt;
      }
    }

    // 4) advance global route s by the *used* part of this lane
    route_s_accum += section_len;
  }

  // 5) optional thinning in s
  for( auto it = reference_line.begin(); it != reference_line.end(); )
  {
    auto next_it = std::next( it );
    if( next_it != reference_line.end() && std::fabs( next_it->first - it->first ) < 0.5 )
    {
      reference_line.erase( it );
      it = next_it;
    }
    else
    {
      ++it;
    }
  }
}

} // namespace map
} // namespace adore
