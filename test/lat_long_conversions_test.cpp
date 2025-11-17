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

#include "adore_map/lat_long_conversions.hpp"

#include <cmath>
#include <gtest/gtest.h>

#include <string>
#include <vector>

namespace
{

struct City
{
  const char* name;
  double      lat;
  double      lon;
};

// Same cities as the old test program
const City kCities[] = {
  {          "San Francisco, CA",  37.7749, -122.4194 },
  {            "Los Angeles, CA",  34.0522, -118.2437 },
  {            "Berlin, Germany",  52.5200,   13.4050 },
  { "Johannesburg, South Africa", -26.2041,   28.0473 },
};

constexpr double kTolDeg = 1e-5; // ≈1 m in latitude, a bit more in longitude

} // namespace

// -----------------------------------------------------------------------------
// C++ PROJ-based implementation
// -----------------------------------------------------------------------------

TEST( LatLongConversionsCpp, RoundTripMajorCities )
{
  for( const auto& city : kCities )
  {
    // Lat/Lon -> UTM
    std::vector<double> utm = adore::map::convert_lat_lon_to_utm( city.lat, city.lon );
    ASSERT_EQ( utm.size(), 4u ) << "Unexpected UTM vector size for " << city.name;

    const double easting   = utm[0];
    const double northing  = utm[1];
    const int    zone      = static_cast<int>( utm[2] );
    const char   zone_char = static_cast<char>( utm[3] );

    // Basic sanity check that we didn't just get all zeros back
    ASSERT_FALSE( easting == 0.0 && northing == 0.0 && zone == 0 ) << "UTM looks invalid (all zeros) for " << city.name;

    // UTM -> Lat/Lon
    std::vector<double> ll = adore::map::convert_utm_to_lat_lon( easting, northing, zone, std::string( 1, zone_char ) );

    ASSERT_EQ( ll.size(), 2u ) << "Unexpected Lat/Lon vector size for " << city.name;

    const double lat_rt = ll[0];
    const double lon_rt = ll[1];

    EXPECT_NEAR( lat_rt, city.lat, kTolDeg ) << "Latitude mismatch for " << city.name;
    EXPECT_NEAR( lon_rt, city.lon, kTolDeg ) << "Longitude mismatch for " << city.name;
  }
}

// -----------------------------------------------------------------------------
// Python-based implementation via shell (optional / disabled by default)
// -----------------------------------------------------------------------------
//
// These rely on:
//   * python3 being available
//   * the `utm` Python module installed
//
// That’s fragile for CI, so we mark them DISABLED_ so they only run if you
// explicitly enable disabled tests via gtest filters.
//

TEST( LatLongConversionsPython, DISABLED_RoundTripMajorCities )
{
  for( const auto& city : kCities )
  {
    // Lat/Lon -> UTM (Python)
    std::vector<double> utm = adore::map::convert_lat_lon_to_utm_python( city.lat, city.lon );
    ASSERT_EQ( utm.size(), 4u ) << "Unexpected UTM vector size for " << city.name;

    const double easting   = utm[0];
    const double northing  = utm[1];
    const int    zone      = static_cast<int>( utm[2] );
    const char   zone_char = static_cast<char>( utm[3] );

    ASSERT_FALSE( easting == 0.0 && northing == 0.0 && zone == 0 ) << "Python UTM looks invalid (all zeros) for " << city.name;

    // UTM -> Lat/Lon (Python)
    std::vector<double> ll = adore::map::convert_utm_to_lat_lon_python( easting, northing, zone, std::string( 1, zone_char ) );

    ASSERT_EQ( ll.size(), 2u ) << "Unexpected Lat/Lon vector size for " << city.name;

    const double lat_rt = ll[0];
    const double lon_rt = ll[1];

    EXPECT_NEAR( lat_rt, city.lat, kTolDeg ) << "Latitude mismatch for " << city.name;
    EXPECT_NEAR( lon_rt, city.lon, kTolDeg ) << "Longitude mismatch for " << city.name;
  }
}

TEST( LatLongConversions, DISABLED_CppAndPythonUTMAgreeApproximately )
{
  constexpr double kTolMeters = 1e-3; // relax if you see small numeric differences

  for( const auto& city : kCities )
  {
    std::vector<double> utm_cpp = adore::map::convert_lat_lon_to_utm( city.lat, city.lon );
    std::vector<double> utm_py  = adore::map::convert_lat_lon_to_utm_python( city.lat, city.lon );

    ASSERT_EQ( utm_cpp.size(), 4u );
    ASSERT_EQ( utm_py.size(), 4u );

    const double e_cpp = utm_cpp[0];
    const double n_cpp = utm_cpp[1];
    const int    z_cpp = static_cast<int>( utm_cpp[2] );
    const char   c_cpp = static_cast<char>( utm_cpp[3] );

    const double e_py = utm_py[0];
    const double n_py = utm_py[1];
    const int    z_py = static_cast<int>( utm_py[2] );
    const char   c_py = static_cast<char>( utm_py[3] );

    ASSERT_FALSE( e_cpp == 0.0 && n_cpp == 0.0 && z_cpp == 0 ) << "CPP UTM looks invalid (all zeros) for " << city.name;
    ASSERT_FALSE( e_py == 0.0 && n_py == 0.0 && z_py == 0 ) << "Python UTM looks invalid (all zeros) for " << city.name;

    EXPECT_NEAR( e_cpp, n_py, kTolMeters ) << "Easting mismatch for " << city.name;
    EXPECT_NEAR( n_cpp, n_py, kTolMeters ) << "Northing mismatch for " << city.name;
    EXPECT_EQ( z_cpp, z_py ) << "Zone mismatch for " << city.name;
    EXPECT_EQ( c_cpp, c_py ) << "Zone letter mismatch for " << city.name;
  }
}
