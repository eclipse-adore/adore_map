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
#include "adore_map/border.hpp"
#include "adore_map/map_point.hpp"

namespace adore
{
namespace map
{

// Remove duplicate points from a vector of MapPoints based on s values
inline static void
remove_duplicate_points( std::vector<MapPoint>& points )
{
  auto last = std::unique( points.begin(), points.end(),
                           []( const MapPoint& a, const MapPoint& b ) { return std::abs( a.s - b.s ) < 1e-6; } );
  points.erase( last, points.end() );
}

} // namespace map

namespace r2s
{

/** @brief Rounds a double to six decimal places.
 */
inline static double round_to_six_decimal_places( const double number ) {
  return std::round( number * 1000000 ) / 1000000;
}

} // namespace r2s

} // namespace adore
