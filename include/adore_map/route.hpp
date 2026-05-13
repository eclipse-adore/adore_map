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
#include <stdlib.h>

#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <algorithm>  //new
#include <cmath>  //new 
#include <limits> //new

#include "adore_map/lane.hpp"
#include "adore_map/map.hpp"
#include "adore_map/quadtree.hpp"
#include "adore_map/r2s_parser.h"
#include "adore_map/road_graph.hpp"
#include "adore_math/distance.h"
#include "adore_math/point.h"
#include "adore_math/pose.h"

namespace adore
{
namespace map
{

// new struct
struct RouteSection
{
  size_t lane_id;
  double route_s;
  double start_s;
  double end_s;
};

struct Route
{
  Route() {};
  std::unordered_map<size_t, std::shared_ptr<RouteSection>> lane_to_sections;
  std::deque<std::shared_ptr<RouteSection>>                 sections;
  std::shared_ptr<Map>                                      map;
  adore::math::Point2d                                      start;
  adore::math::Point2d                                      destination;
  std::map<double, MapPoint>                                reference_line;

  double               get_length() const;
  void                 add_route_section( Border& points, const MapPoint& start_point, const MapPoint& end_point, bool reverse );
  std::deque<MapPoint> get_shortened_route( double start_s, double desired_length ) const;
  MapPoint             get_map_point_at_s( double distance ) const;
  math::Pose2d         get_pose_at_s( double distance ) const;
  double               get_curvature_at_s( double s ) const;
  void                 initialize_reference_line();

  template<typename StartPoint, typename EndPoint>
  Route( const StartPoint& start_point, const EndPoint& end, const std::shared_ptr<Map>& reference_map );

  template<typename State>
  double get_s( const State& state ) const;

  template<typename TPoint>
  TPoint interpolate_at_s( double distance ) const;

  template<typename PLike>
  double refine_s_with_arc( const PLike& pos, double coarse_s ) const;
};

template<typename StartPoint, typename EndPoint>
Route::Route( const StartPoint& start_point, const EndPoint& end, const std::shared_ptr<Map>& reference_map )
{
  start.x       = start_point.x;
  start.y       = start_point.y;
  destination.x = end.x;
  destination.y = end.y;
  map           = reference_map;

  // Find nearest start and end points using the quadtree
  double min_start_dist      = std::numeric_limits<double>::max();
  auto   nearest_start_point = map->quadtree.get_nearest_point( start, min_start_dist );

  double min_end_dist      = std::numeric_limits<double>::max();
  auto   nearest_end_point = map->quadtree.get_nearest_point( end, min_end_dist );


  if( nearest_start_point && nearest_end_point )
  {
    size_t start_lane_id = nearest_start_point->parent_id;
    size_t end_lane_id   = nearest_end_point->parent_id;

    // Find the best path between the start and end lanes
    auto lane_id_route = map->lane_graph.get_best_path( start_lane_id, end_lane_id );

    // Iterate over the route and process each lane
    for( size_t i = 0; i < lane_id_route.size(); ++i )
    {
      auto lane = map->lanes.at( lane_id_route[i] );
      add_route_section( lane->borders.center, *nearest_start_point, *nearest_end_point, lane->left_of_reference );
    }

    initialize_reference_line();
  }
}

// get distance to object along route and if the object is within the lane
template<typename State>
double
Route::get_s( const State& state ) const
{
  if( !map ) // no map => we cannot proceed
  {
    std::cerr << "route needs map!" << std::endl;
    return std::numeric_limits<double>::infinity();
  }

  double min_dist = std::numeric_limits<double>::max();
  auto   nearest  = map->quadtree.get_nearest_point( state, min_dist, [&]( const MapPoint& p ) {
    // Return true only if p's lane_id is in our route_lane_ids
    return ( lane_to_sections.find( p.parent_id ) != lane_to_sections.end() );
  } );

  // If we didn't find any point that meets the filter
  if( !nearest )
  {
    std::cerr << "no nearest     state x " << state.x << " y " << state.y << std::endl;
    return std::numeric_limits<double>::infinity();
  }

  auto near_sec = lane_to_sections.at( nearest->parent_id );

  double dist_along_sec = near_sec->start_s < near_sec->end_s ? ( nearest->s - near_sec->start_s ) : near_sec->start_s - nearest->s;

  //double route_distance = near_sec->route_s + dist_along_sec;
  //return refine_s_with_arc( *nearest, route_distance );
  double route_distance = near_sec->route_s + dist_along_sec;
  return refine_s_with_arc( state, route_distance );
}

template<typename TPoint>
TPoint
Route::interpolate_at_s( double distance ) const
{
  TPoint result;

  // Early exit for empty or single-point lanes
  if( reference_line.empty() )
  {
    return result;
  }

  if( reference_line.size() == 1 )
  {
    const auto& sp = reference_line.begin()->second;
    result.x       = sp.x;
    result.y       = sp.y;

    // If T has a member called 'yaw', set it to 0.0
    if constexpr( requires { result.yaw; } )
    {
      result.yaw = 0.0;
    }
    return result;
  }

  // Interpolation logic
  auto upper_it = reference_line.lower_bound( distance );
  auto lower_it = upper_it;

  double frac = 0.0;

  if( upper_it == reference_line.end() )
  {
    upper_it--;
    lower_it = std::prev( upper_it );
    frac     = 1.0;
  }
  else if( upper_it == reference_line.begin() )
  {
    upper_it++;
    frac = 0.0;
  }
  else
  {
    lower_it--;
    double s1    = lower_it->first;
    double s2    = upper_it->first;
    double denom = ( s2 - s1 );
    frac         = ( std::fabs( denom ) < 1e-9 ) ? 0.0 : ( distance - s1 ) / denom;
  }

  // Linear Interpolation
  double x1 = lower_it->second.x;
  double y1 = lower_it->second.y;
  double x2 = upper_it->second.x;
  double y2 = upper_it->second.y;

  result.x = x1 + frac * ( x2 - x1 );
  result.y = y1 + frac * ( y2 - y1 );

  // Yaw calculation only if T has 'yaw'
  if constexpr( requires { result.yaw; } )
  {
    double dx = x2 - x1;
    double dy = y2 - y1;
    if( !( std::fabs( dx ) < 1e-9 && std::fabs( dy ) < 1e-9 ) )
    {
      result.yaw = std::atan2( dy, dx );
    }
  }

  if constexpr( requires {
                  result.s;
                  result.parent_id;
                  result.max_speed;
                } )
  {
    auto upper_it = reference_line.lower_bound( distance );
    auto lower_it = upper_it;

    if( upper_it == reference_line.end() )
    {
      upper_it--;
      lower_it = std::prev( upper_it );
    }
    else if( upper_it == reference_line.begin() )
    {
      lower_it = upper_it;
    }
    else
    {
      lower_it = std::prev( upper_it );
    }

    double dist_to_lower = std::abs( distance - lower_it->first );
    double dist_to_upper = std::abs( distance - upper_it->first );
    auto   nearest_it    = ( dist_to_lower < dist_to_upper ) ? lower_it : upper_it;

    result.parent_id = nearest_it->second.parent_id;
    result.max_speed = nearest_it->second.max_speed;
    result.s         = distance;
  }

  return result;
}

template<typename PoseT>
Route
get_default_route( const PoseT& start_pose, double max_length, const std::shared_ptr<Map>& map )
{
  Route route;
  route.map = map;

  double min_start_dist = std::numeric_limits<double>::max();
  auto   nearest_point  = map->quadtree.get_nearest_point( start_pose, min_start_dist );

  double length          = 0;
  size_t current_lane_id = nearest_point->parent_id;

  while( length < max_length && map->lane_graph.to_successors.count( current_lane_id ) )
  {
    auto lane = map->lanes.at( current_lane_id );
    route.add_route_section( lane->borders.center, *nearest_point, MapPoint(), lane->left_of_reference );
    length += lane->length;

    std::cerr << "route length " << length << " max " << max_length << std::endl;
    auto& lane_ids = map->lane_graph.to_successors.at( current_lane_id );
    if( lane_ids.size() == 0 )
      break;

    current_lane_id = *lane_ids.begin(); // just take first connecting lane
  }

  route.initialize_reference_line();

  return route;
}

/* -----------------------------------------------------------
 *  continuous refinement via local circular-arc
 * -----------------------------------------------------------*/
template<typename PLike>
double
Route::refine_s_with_arc( const PLike& pos, double coarse_s ) const
{
  /* 0.  need at least three samples                                    */
  if( reference_line.size() < 3 || !std::isfinite( coarse_s ) )
  {
    return coarse_s;
  }

  /* 1.  locate the nearest MapPoint on the 1-m centre-lane grid        */
  auto it_hi = reference_line.lower_bound( coarse_s );

  auto nearest =
    ( it_hi == reference_line.end() )
      ? std::prev( it_hi )
      : ( it_hi == reference_line.begin() )
          ? it_hi
          : ( std::abs( coarse_s - std::prev( it_hi )->first ) <
              std::abs( coarse_s - it_hi->first ) )
              ? std::prev( it_hi )
              : it_hi;

  if( nearest == reference_line.begin() )
  {
    return coarse_s;
  }

  auto it_prev = std::prev( nearest );
  auto it_next = std::next( nearest );

  if( it_next == reference_line.end() )
  {
    return coarse_s;
  }

  const MapPoint& p0 = it_prev->second;
  const MapPoint& p1 = nearest->second;
  const MapPoint& p2 = it_next->second;

  double p0_s = it_prev->first;
  //double p1_s = nearest->first;
  double p2_s = it_next->first;


  /* 2.  fit circle through p0-p1-p2 (barycentric formula)              */
  const double a =
    p0.x * ( p1.y - p2.y )
    - p0.y * ( p1.x - p2.x )
    + p1.x * p2.y
    - p2.x * p1.y;

  // Nearly straight local segment: do not force an unstable circle fit.
  if( std::fabs( a ) < 1e-6 )
  {
    return coarse_s;
  }

  const double sq0 = p0.x * p0.x + p0.y * p0.y;
  const double sq1 = p1.x * p1.x + p1.y * p1.y;
  const double sq2 = p2.x * p2.x + p2.y * p2.y;

  const double cx =
    ( sq0 * ( p2.y - p1.y )
    + sq1 * ( p0.y - p2.y )
    + sq2 * ( p1.y - p0.y ) ) / ( 2.0 * a );

  const double cy =
    ( sq0 * ( p1.x - p2.x )
    + sq1 * ( p2.x - p0.x )
    + sq2 * ( p0.x - p1.x ) ) / ( 2.0 * a );

  const double r = std::hypot( p1.x - cx, p1.y - cy );

  if( !std::isfinite( cx ) || !std::isfinite( cy ) || !std::isfinite( r ) )
  {
    return coarse_s;
  }

  /* 3.  radial projection of ego position onto the circle              */
  const double dx = pos.x - cx;
  const double dy = pos.y - cy;
  const double n2 = dx * dx + dy * dy;

  if( n2 < 1e-9 )
  {
    return coarse_s;
  }

  const double k = r / std::sqrt( n2 );
  const double px = cx + k * dx;
  const double py = cy + k * dy;

  auto ang = [&]( double x, double y )
  {
    return std::atan2( y - cy, x - cx );
  };

  auto norm = []( double angle )
  {
    while( angle > M_PI )
    {
      angle -= 2.0 * M_PI;
    }
    while( angle < -M_PI )
    {
      angle += 2.0 * M_PI;
    }
    return angle;
  };

  const double a0 = ang( p0.x, p0.y );
  const double a2 = ang( p2.x, p2.y );
  const double ap = ang( px, py );

  const double theta = norm( a2 - a0 );
  const double phi   = norm( ap - a0 );

  if( !std::isfinite( theta ) ||
      !std::isfinite( phi ) ||
      std::fabs( theta ) < 1e-9 )
  {
    return coarse_s;
  }

  const bool inside_arc =
    theta >= 0.0
      ? ( phi >= 0.0 && phi <= theta )
      : ( phi <= 0.0 && phi >= theta );

  if( !inside_arc )
  {
    return coarse_s;
  }

  /* 4.  convert angular offset to station                              */
  double ratio = phi / theta;
  ratio = std::clamp( ratio, 0.0, 1.0 );

  const double refined_s = p0_s + ratio * ( p2_s - p0_s );

  if( !std::isfinite( refined_s ) )
  {
    return coarse_s;
  }

  return refined_s;
}

// function to get s on route for a given state, used for shifted routes in obstacle avoidance
template <typename State>
double get_s_on_reference_line_segments(
  const map::Route& route,
  const State& state,
  double coarse_s,
  double search_window = 20.0 )
{
  if( route.reference_line.size() < 2 )
  {
    return std::numeric_limits<double>::infinity();
  }

  const double s_min = coarse_s - search_window;
  const double s_max = coarse_s + search_window;

  auto it = route.reference_line.lower_bound( s_min );
  if( it == route.reference_line.end() )
  {
    return route.reference_line.rbegin()->first;
  }

  if( it != route.reference_line.begin() )
  {
    --it;
  }

  double best_s = it->first;
  double best_d2 = std::numeric_limits<double>::max();

  auto next = std::next( it );

  for( ; next != route.reference_line.end(); ++it, ++next )
  {
    const double s0 = it->first;
    const double s1 = next->first;

    if( s0 > s_max )
    {
      break;
    }

    const auto& p0 = it->second;
    const auto& p1 = next->second;

    const double vx = p1.x - p0.x;
    const double vy = p1.y - p0.y;
    const double wx = state.x - p0.x;
    const double wy = state.y - p0.y;

    const double len2 = vx * vx + vy * vy;
    if( len2 < 1e-9 )
    {
      continue;
    }

    double t = ( wx * vx + wy * vy ) / len2;
    t = std::clamp( t, 0.0, 1.0 );

    const double px = p0.x + t * vx;
    const double py = p0.y + t * vy;

    const double dx = state.x - px;
    const double dy = state.y - py;
    const double d2 = dx * dx + dy * dy;

    if( d2 < best_d2 )
    {
      best_d2 = d2;
      best_s = s0 + t * ( s1 - s0 );
    }
  }

  return best_s;
}

} // namespace map
} // namespace adore
