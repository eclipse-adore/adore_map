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

#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

#include "adore_map/lane.hpp"
#include "adore_map/map.hpp"
#include "adore_map/map_loader.hpp"
#include "adore_map/road_graph.hpp"
#include "adore_map/config.hpp"

#ifndef ADORE_MAP_TEST_DATA_DIR
  // Fallback – will be overridden from CMake for real tests.
  #define ADORE_MAP_TEST_DATA_DIR "."
#endif

namespace
{
std::string
get_test_map_r2s_cfg_path()
{
  return std::string( ADORE_MAP_TEST_DATA_DIR ) + "/r2s_wfs_config_bs.json";
}

std::string
get_test_map_r2s_r2sr_path()
{
  return std::string( ADORE_MAP_TEST_DATA_DIR ) + "/de_bs_borders_wfs.r2sr";
}

std::string
get_test_map_r2s_file_cache_path()
{
  return std::string( ADORE_MAP_TEST_DATA_DIR ) + "/cache";
}

std::stringstream* buffer;
std::streambuf* sbuf;

// Test fixture for MapDownloadCache tests, with setup and teardown to suppress output during tests
// All tests use a local file cache, so no actual downloading from a server is performed
// Tests focus on verifying that the caching mechanism correctly saves and loads map data, 
// and that the loaded map data is consistent with expectations
// For test programs actually downloading from a server, see directory adore/adore_test_programs/adore_map_downloader_test
// The executables of the tests in the aforementioned directory can be found in .colcon_workspace/install/adore_map_downloader_test/bin
class MapDownloadCacheTest : public testing::Test 
{
  protected:
  // Per-test-suite set-up that suppresses output to std::cout during tests
  // to prevent cluttering the test output with download and parsing logs
  // Output is restored after all tests in this suite have run
  // Called before the first test in this test suite
  static void
  SetUpTestSuite() {
    std::cout << "MapDownloadCacheTest: Suppressing output to std::cout during tests." << std::endl;
    buffer = new std::stringstream();
    sbuf = std::cout.rdbuf();
    // Redirect cout to the buffer to suppress output during tests
    std::cout.rdbuf( buffer->rdbuf() );
  }

  // Per-test-suite tear-down that restores output to std::cout after tests
  // Called after the last test in this test suite
  static void
  TearDownTestSuite() {
    // Restore cout's original buffer
    std::cout.rdbuf( sbuf );
    // Clean up the buffer
    delete buffer;
    buffer = nullptr;
    std::cout << "MapDownloadCacheTest: Output to std::cout restored after tests." << std::endl;
  }

  // In-Test helper functions

  // Modifies a filename by replacing the last dot with an underscore and appending a new end with suffix
  static std::string
  modified_filename( const std::string filename, const std::string new_end_with_suffix ) 
  {
    std::string str = filename;
    size_t last_dot_pos = str.find_last_of( '.' );
    if( last_dot_pos != std::string::npos )
    {
      str[ last_dot_pos ] = '_';
    }
    std::string modified_filename = str + new_end_with_suffix;
    return modified_filename;
  }

  // Compares two files for identical content
  static bool
  are_identical_files( const std::string& file1, const std::string& file2 )
  {
    // If under linux, call diff -a, if under windows, call fc
    // to compare the two JSON files
    #ifdef _WIN32
      std::string command = "fc \"" + file1 + "\" \"" + file2 + "\"";
    #else
      std::string command = "diff -a \"" + file1 + "\" \"" + file2 + "\"";
    #endif
    int ret = system( command.c_str() );
    return ret == 0;
  }
};
} // namespace

// Basic smoke test: we can load the map and it has roads, lanes and a lane graph.
TEST_F( MapDownloadCacheTest, loaded_map_has_roads_lanes_and_graph )
{
  Config cfg( get_test_map_r2s_cfg_path() );
  MapDownloader map_downloader( cfg, get_test_map_r2s_file_cache_path() );
  adore::map::Map map = adore::map::MapLoader::download_from_wfs( map_downloader, cfg.layer_name_reference_lines, 
    cfg.layer_name_lane_borders, false );

  // Structural checks
  EXPECT_FALSE( map.roads.empty() ) << "Expected at least one road in the loaded map";
  EXPECT_FALSE( map.lanes.empty() ) << "Expected at least one lane in the loaded map";

  EXPECT_FALSE( map.lane_graph.to_successors.empty() ) << "Lane graph should have successors";
  EXPECT_FALSE( map.lane_graph.to_predecessors.empty() ) << "Lane graph should have predecessors";
  EXPECT_FALSE( map.lane_graph.all_connections.empty() ) << "Lane graph should contain connections";

  // Take the first lane and do some sanity checks.
  auto lane_it = map.lanes.begin();
  ASSERT_NE( lane_it, map.lanes.end() );
  const size_t                             lane_id = lane_it->first;
  const std::shared_ptr<adore::map::Lane>& lane    = lane_it->second;
  ASSERT_TRUE( lane );

  EXPECT_GT( lane->length, 0.0 ) << "Lane length should be > 0";

  // get_lane_speed_limit must return a positive speed for a valid lane.
  const double speed_limit = map.get_lane_speed_limit( lane_id );
  EXPECT_GT( speed_limit, 0.0 );

  // The lane's road_id must refer to an existing road.
  auto road_it = map.roads.find( lane->road_id );
  ASSERT_NE( road_it, map.roads.end() ) << "Lane's road_id does not exist in map.roads";
}

// Check that all lane-graph connections refer to lanes that actually exist in the map.
TEST_F( MapDownloadCacheTest, lane_graph_connections_reference_existing_lanes )
{
  Config cfg( get_test_map_r2s_cfg_path() );
  MapDownloader map_downloader( cfg, get_test_map_r2s_file_cache_path() );
  adore::map::Map map = adore::map::MapLoader::download_from_wfs( map_downloader, cfg.layer_name_reference_lines, 
    cfg.layer_name_lane_borders, false );

  for( const auto& connection : map.lane_graph.all_connections )
  {
    EXPECT_TRUE( map.lanes.count( connection.from_id ) > 0 ) << "Missing lane for connection.from_id " << connection.from_id;
    EXPECT_TRUE( map.lanes.count( connection.to_id ) > 0 ) << "Missing lane for connection.to_id " << connection.to_id;
  }
}

// Ensure that the quadtree actually contains points from at least one loaded lane.
TEST_F( MapDownloadCacheTest, quadtree_contains_points_from_lanes )
{
  Config cfg( get_test_map_r2s_cfg_path() );
  MapDownloader map_downloader( cfg, get_test_map_r2s_file_cache_path() );
  adore::map::Map map = adore::map::MapLoader::download_from_wfs( map_downloader, cfg.layer_name_reference_lines, 
    cfg.layer_name_lane_borders, false );

  // Pick any lane.
  auto lane_it = map.lanes.begin();
  ASSERT_NE( lane_it, map.lanes.end() );
  const std::shared_ptr<adore::map::Lane>& lane = lane_it->second;
  ASSERT_TRUE( lane );

  // Use one of its center-line points as a query center.
  const auto& center_points = lane->borders.center.interpolated_points;
  ASSERT_FALSE( center_points.empty() ) << "Expected interpolated center points in lane borders";

  const auto& query_point = center_points.front();

  std::vector<adore::map::MapPoint> found;
  map.quadtree.query_range( query_point, 1.0, found );

  EXPECT_FALSE( found.empty() ) << "Quadtree query around a lane center point returned no points";
}

// Ensure that the reference line and lane border data loaded from WFS matches the data loaded from files (which had been downloaded by a Python program).
TEST_F( MapDownloadCacheTest, results_comparable_to_legacy_results )
{
  Config cfg( get_test_map_r2s_cfg_path() );
  MapDownloader map_downloader( cfg, get_test_map_r2s_file_cache_path() );
  adore::map::Map map = adore::map::MapLoader::download_from_wfs( map_downloader, cfg.layer_name_reference_lines, 
    cfg.layer_name_lane_borders, false );

  // Load reference line and lane border data from CSV files created using a Python downloader and the R2S parser 
  // (the legacy methods for loading map data). These files were created from the same source as the WFS data, so they should match.
  auto border_data_r2sr_from_file = adore::r2s::load_border_data_from_r2sr_file( get_test_map_r2s_r2sr_path() );
  // The same path is passed to the next method since the method will change the suffix to .r2sl internally 
  auto border_data_r2sl_from_file = adore::r2s::load_border_data_from_r2sl_file( get_test_map_r2s_r2sr_path() );

  auto border_data_r2sr_from_wfs = adore::r2s::download_reference_lines( map_downloader, cfg.layer_name_reference_lines );
  auto border_data_r2sl_from_wfs = adore::r2s::download_lane_borders( map_downloader, cfg.layer_name_lane_borders );
  
  // Next line uses operator!= defined in BorderDataR2SR, which uses a tolerance for comparing x and y coordinates
  EXPECT_FALSE( border_data_r2sr_from_file != border_data_r2sr_from_wfs )
    << "Reference line data loaded from WFS does not match data loaded from file";
  // Next line uses operator!= defined in BorderDataR2SL, which uses a tolerance for comparing x and y coordinates
  EXPECT_FALSE( border_data_r2sl_from_file != border_data_r2sl_from_wfs )
    << "Lane border data loaded from WFS does not match data loaded from file";
}

TEST_F( MapDownloadCacheTest, files_match_after_saving_loading_and_saving_again )
{
  Config cfg( get_test_map_r2s_cfg_path() );
  MapDownloader map_downloader( cfg, get_test_map_r2s_file_cache_path() );

  // Load the first map layer as JSON: layer name is that for reference lines
  ASSERT_TRUE( map_downloader.download( cfg.layer_name_reference_lines, cfg.bbox ) ) << "Failed to download reference lines";
  // Create a JSON file for reference lines
  map_downloader.save( cfg.reference_line_filename );
  // Load a second map layer as JSON: lane borders
  ASSERT_TRUE( map_downloader.download( cfg.layer_name_lane_borders, cfg.bbox ) ) << "Failed to download lane borders";
  // Create a JSON file for lane borders
  map_downloader.save( cfg.lane_border_filename );
  
  // Now test the flexible methods with more parameters
  // Load the first map layer as JSON: reference lines
  ASSERT_TRUE( map_downloader.download( cfg.server_url, cfg.project_name, cfg.target_srs,
      cfg.layer_name_reference_lines, cfg.bbox ) ) << "Failed to download reference lines";
  // Get the parsed JSON data for reference lines
  nlohmann::json& reference_line_data = map_downloader.get_json_data();
  // Create a JSON file for reference lines
  std::string reference_line_downloaded_with_flexible_method_filename 
    = modified_filename( cfg.reference_line_filename, "_downloaded_with_flexible_method.json" );
  map_downloader.save( reference_line_data, reference_line_downloaded_with_flexible_method_filename );
  ASSERT_TRUE( are_identical_files( cfg.reference_line_filename, reference_line_downloaded_with_flexible_method_filename ) ) 
    << "The two JSON files for reference lines (from the simple and the flexible method) differ.";
  
  // Load the map layer from file instead of downloading it: reference lines
  map_downloader.load( cfg.reference_line_filename, reference_line_data );
  // Save again as JSON file for reference lines
  std::string reference_line_loaded_filename = modified_filename( cfg.reference_line_filename, "_loaded.json" );
  map_downloader.save( reference_line_data, reference_line_loaded_filename );
  ASSERT_TRUE( are_identical_files( cfg.reference_line_filename, reference_line_loaded_filename ) ) 
    << "The two JSON files for reference lines (from the simple method and from loading and saving) differ.";

  // Clean up: remove the created JSON files for reference lines
  std::remove( cfg.reference_line_filename.c_str() );
  std::remove( reference_line_loaded_filename.c_str() );
  std::remove( reference_line_downloaded_with_flexible_method_filename.c_str() );

  // Load another map layer as JSON: lane borders
  ASSERT_TRUE( map_downloader.download( cfg.server_url, cfg.project_name, cfg.target_srs,
      cfg.layer_name_lane_borders, cfg.bbox ) ) << "Failed to download lane borders";
  // Get the parsed JSON data for lane borders
  nlohmann::json& lane_border_data = map_downloader.get_json_data();
  // Create a JSON file for lane borders
  std::string lane_border_downloaded_with_flexible_method_filename 
    = modified_filename( cfg.lane_border_filename, "_downloaded_with_flexible_method.json" );
  map_downloader.save( lane_border_data, lane_border_downloaded_with_flexible_method_filename );
  ASSERT_TRUE( are_identical_files( cfg.lane_border_filename, lane_border_downloaded_with_flexible_method_filename ) ) 
    << "The two JSON files for lane borders (from the simple and the flexible method) differ.";
    
  // Load the map layer from file instead of downloading it: lane borders
  map_downloader.load( cfg.lane_border_filename, lane_border_data );
  // Save again as JSON file for lane borders
  std::string lane_border_loaded_filename = modified_filename( cfg.lane_border_filename, "_loaded.json" );
  map_downloader.save( lane_border_data, lane_border_loaded_filename );
  ASSERT_TRUE( are_identical_files( cfg.lane_border_filename, lane_border_loaded_filename ) ) 
    << "The two JSON files for lane borders (from the simple method and from loading and saving) differ.";
  
  // Clean up: remove the created JSON files for lane borders
  std::remove( cfg.lane_border_filename.c_str() );
  std::remove( lane_border_loaded_filename.c_str() );
  std::remove( lane_border_downloaded_with_flexible_method_filename.c_str() );
}
