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

#include <iomanip>
#include <iostream>
#include <string>

namespace adore
{
namespace map
{
struct MapPoint
{
  MapPoint( double x, double y, size_t parent_id ) :
    x( x ),
    y( y ),
    parent_id( parent_id ) {};

  MapPoint() {};
  double                x         = 666;
  double                y         = 420;
  double                s         = 0; // along the lane
  size_t                parent_id = 0; // id of the LANE to which it belongs
  std::optional<double> max_speed = std::nullopt;

  bool
  operator==( const MapPoint& other ) const
  {
    return ( x == other.x && y == other.y );
  }

  bool
  operator!=( const MapPoint& other ) const
  {
    return !( other == *this );
  }

  friend std::ostream&
  operator<<( std::ostream& os, const MapPoint& mp )
  {
    // Save the original formatting flags and precision
    std::ios oldState( nullptr );
    oldState.copyfmt( os );

    // Set the desired number of decimal places
    os << std::fixed << std::setprecision( 2 ); // Adjust '2' to the number of decimal places you want

    os << "x: " << mp.x << ", y: " << mp.y << ", s: " << mp.s << ", parent_id: " << mp.parent_id;

    // Restore the original formatting flags and precision
    os.copyfmt( oldState );

    return os;
  }
};

// using MapPointPtr = std::shared_ptr<MapPoint>;
} // namespace map
} // namespace adore
