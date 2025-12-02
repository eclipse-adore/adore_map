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
#include <cstdio>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <caches/lru_cache_policy.hpp>
#include "adore_map/xcache.hpp"
#include "adore_map/json_file_helpers.hpp"

// Alias for an easy class typing
template <typename Key, typename Value>
using lru_cache_t = typename caches::fixed_sized_cache<Key, Value, caches::LRUCachePolicy>;

template <typename Key, typename Value>
using lru_xcache_t = typename caches::XCache<Key, Value, caches::LRUCachePolicy>;

/**
 * @brief MapCache is a two-level cache system with RAM and disk caching for map data.
 * @note This class uses LRU cache policy for both RAM and disk caches.
 * @note The disk cache stores map data as files in a specified directory, with filenames
 *       corresponding to unique keys. The RAM cache holds the actual map data in memory for quick access.
 * @details The MapCache class provides a two-level caching mechanism for map data, utilizing
 *          both RAM and disk storage. It employs LRU (Least Recently Used) cache policy for both
 *          levels of caching to optimize data retrieval and storage efficiency. Items evicted from RAM 
 *          cache will be inserted into disk cache and items on disk will be inserted back to 
 *          RAM cache upon lookup. Cache contents are saved to disk upon exit (by the destructor), 
 *          and reloaded for the next session (by the constructor). All of these transitions are transparent 
 *          to users.
 */
class MapCache
{
public:

  /** @brief Default MapCache constructor
   * @details This constructor initializes an empty MapCache with default parameters.
   * @note A directory for disk cache must be set up later using set_up_file_cache_path() before using the cache.
   */
  MapCache() : my_file_cache_path( "" ), ram_cache_size( 64 ), disk_cache_size( 256 ), entry_count( 0 ), 
    on_final_clear( false ), is_active( true ), debug_mode( false ), 
    ram_cache( ram_cache_size, caches::LRUCachePolicy<std::string>(),
      [this]( const std::string& key,
        const lru_cache_t<std::string, nlohmann::json>::value_type& value_handle ) 
      {
        this->on_erase_callback_for_ram_cache( key, value_handle );
      }),
    disk_cache( disk_cache_size, caches::LRUCachePolicy<std::string>(),
      [this]( const std::string& key,
        const lru_cache_t<std::string, int>::value_type& value_handle ) 
      {
        this->on_erase_callback_for_disk_cache( key, value_handle );
      })
  {
  };

  /** @brief MapCache constructor
   * @param[in] file_cache_path Path to the directory where disk cache files will be stored
   * @param[in] ram_cache_size Maximum size of the RAM cache (in number of entries)
   * @param[in] disk_cache_size Maximum size of the disk cache (in number of entries)
   * @param[in] active Boolean flag to activate or deactivate the cache
   * @param[in] debug Boolean flag to enable or disable debug mode
   */
  // MapCache( std::string file_cache_path, std::size_t ram_cache_size = 64, std::size_t disk_cache_size = 256, 
  //   bool active = true, bool debug = false ) :
   MapCache( const std::string& file_cache_path, const std::size_t ram_cache_size = 64, 
    const std::size_t disk_cache_size = 256, const bool active = true, const bool debug = false ) :
    my_file_cache_path( file_cache_path ), ram_cache_size( ram_cache_size ), 
    disk_cache_size( disk_cache_size ), entry_count( 0 ), on_final_clear( false ), 
    is_active( active ), debug_mode( debug ),
    ram_cache( ram_cache_size, caches::LRUCachePolicy<std::string>(),
      [this]( const std::string& key,
        const lru_cache_t<std::string, nlohmann::json>::value_type& value_handle ) 
      {
        this->on_erase_callback_for_ram_cache( key, value_handle );
      }),
    disk_cache( disk_cache_size, caches::LRUCachePolicy<std::string>(),
      [this]( const std::string& key,
        const lru_cache_t<std::string, int>::value_type& value_handle ) 
      {
        this->on_erase_callback_for_disk_cache( key, value_handle );
      })
  {
    set_up_file_cache_path( file_cache_path );
  }

  /** @brief MapCache destructor
   * @details The destructor saves all cache entries to disk by invoking the onEraseCallback
   * for each entry in the disk cache.
   */
  ~MapCache() 
  {
    // Save all cache entries to disk (via onEraseCallback)
    on_final_clear = true;
    if( debug_mode ) 
    {
      // Debugging line to see the final clear operation
      std::cout << "MapCache::~MapCache: Final clear operation, saving disk cache entries to " 
        << my_file_cache_path << "cached.map" << std::endl;
    }
    std::cout << "MapCache::~MapCache: disk cache size: " << disk_cache.Size() << std::endl;
  }

  /** @brief Put a map data entry into the cache
   * @param[in] key Unique key identifying the map data
   * @param[in] value JSON object representing the map data
   */
  void put( const std::string& key, const nlohmann::json& value )
  {
    if( is_active == false ) 
    {
      std::cerr << "MapCache::put: Cache is not active, cannot put item." << std::endl;
      return;
    }
    ram_cache.Put( key, value );
    if( disk_cache.TryGet( key ).second ) 
    {
      if( debug_mode ) 
      {
        // Debugging line to see the key being put into cache
        std::cout << "MapCache::put: Map already exists in disk cache, skipping put operation." << std::endl;
      }
      return;
    }
    disk_cache.Put( key, entry_count );
    if( debug_mode ) 
    {
      // Debugging line to see the file_cache_path and entry_count
      std::cout << "MapCache::put: file_cache_path: " << my_file_cache_path << ", entry_count: " 
        << entry_count << std::endl;
      std::cout << "MapCache::put: Saving entry to disk cache at " << my_file_cache_path
        << "cache.entry_" << entry_count << ".json" << std::endl;
      std::cout << "MapCache::put: Disk cache size: " << disk_cache.Size() << std::endl;
    }
    JsonFileHelpers::save( value, my_file_cache_path + "cache.entry_" + std::to_string( entry_count++ ) 
      + ".json", "MapCache::put" );
  }

  /** @brief Try to get a map data entry from the cache
   * @param[in] key Unique key identifying the map data
   * @return A shared pointer to the JSON object (or nullptr if not found)
   */
  lru_cache_t<std::string, nlohmann::json>::value_type try_get( const std::string& key ) 
  {
    if( is_active == false ) 
    {
      std::cerr << "MapCache::try_get: Cache is not active, cannot get item.\n";
      return nullptr;
    }
    // If the key is empty, return a nullptr
    if( key.empty() ) 
    {
      return nullptr;
    }
    // First, check the RAM cache
    std::pair<lru_cache_t<std::string, nlohmann::json>::value_type, bool> ram_pair = ram_cache.TryGet( key );
    if( ram_pair.second ) 
    {
      assert( ram_pair.first.get() != nullptr );
      return ram_pair.first;
    }
    // If not found in RAM cache, check the disk cache
    std::pair<lru_xcache_t<std::string, int>::value_type, bool> disk_pair = disk_cache.TryGet( key );
    if( !disk_pair.second ) 
    { // Give up if not found in disk cache
      if( debug_mode ) 
      {
        // Debugging line to see that the key was not found in cache
        std::cout << "MapCache::try_get: Key not found in cache: " << key << std::endl;
        // Debugging line to see the file_cache_path
        std::cout << "MapCache::try_get: file_cache_path: " << my_file_cache_path << std::endl;
      }
      return nullptr;
    } 
    else 
    {
      assert( disk_pair.first.get() != nullptr );
      // If found in disk cache, load the value from the file
      if( debug_mode ) 
      {
      // Debugging line to see the file_cache_path and entryCount
        std::cout << "MapCache::try_get: file_cache_path: " << my_file_cache_path
          << ", loaded entryCount: " << *disk_pair.first << std::endl;
        std::cout << "MapCache::try_get: Saving entry to disk cache at "
          << my_file_cache_path << "cache.entry_" << *disk_pair.first << ".json" << std::endl;
      }
      // Load the JSON data from the file
      std::string filename = my_file_cache_path + "cache.entry_" + std::to_string( *disk_pair.first ) + ".json";
      std::shared_ptr<nlohmann::json> json_data_ptr = std::make_shared<nlohmann::json>();
      // Load the JSON data from the file
      JsonFileHelpers::load( filename, *json_data_ptr, "MapCache::try_get" );
      // Insert item back into RAM cache
      ram_cache.Put( key, *json_data_ptr );
      return json_data_ptr;
    }
  }

  /** @brief Turn off the cache
   * @details When the cache is turned off, no cache operations will be performed.
   */
  void turn_off()
  {
    is_active = false;
    if( debug_mode )
    {
      // Debugging line
      std::cout << "MapCache::turn_off: Cache is turned off, no cache operations will be performed." << std::endl;
    }
  }

  /** @brief Turn on the cache
   * @details When the cache is turned on, cache operations will be performed.
   */
  void turn_on()
  {
    is_active = true;
    if( debug_mode )
    {
      // Debugging line
      std::cout << "MapCache::turn_on: Cache is turned on, cache operations will be performed." << std::endl;
    }
  }

  /** @brief Turn on debug mode
   * @details When debug mode is turned on, debugging messages will be sent to stdout.
   */
  void set_debug_mode( const bool& debug )
  {
    debug_mode = debug;
    if( debug_mode ) 
    {
      std::cout << "MapCache::set_debug_mode: debug mode is turned on, debugging messages will be sent to stdout." 
        << std::endl;
    } 
  }

  /** @brief Set up the file cache path
   * @param[in] file_cache_path New path to the directory where disk cache files will be stored
   */
  void set_up_file_cache_path( const std::string& file_cache_path )
  {
    my_file_cache_path = file_cache_path;
    // Ensure the cache directory exists
    if( my_file_cache_path.empty() ) 
    {
      my_file_cache_path = "cache/"; // Use forward slash for cross-platform compatibility
      if( debug_mode ) 
      {
        // Debugging line to see the cache path being used
        std::cout << "MapCache::set_up_file_cache_path: " << my_file_cache_path << std::endl;
      }
    }
    if( !std::filesystem::is_directory( my_file_cache_path ) ) 
    {
      if( debug_mode ) 
      {
        std::cout << "MapCache::set_up_file_cache_path: file che pathdoes not exist: " << my_file_cache_path << std::endl;
        std::cout << "MapCache::set_up_file_cache_path: Creating cache directory at " << my_file_cache_path << std::endl;
      }
      if( !std::filesystem::create_directories( my_file_cache_path ) ) 
      {
        std::cerr << "MapCache::set_up_file_cache_path: Failed to create cache directory: " << my_file_cache_path << '\n';
      }
    }    
    if( !my_file_cache_path.empty() && my_file_cache_path.back() != '/' ) 
    {
      my_file_cache_path += "/";
    }
    if( std::ifstream( my_file_cache_path + "cached.map" ) ) 
    {
      std::ifstream file( my_file_cache_path + "cached.map" );
      if( !file.is_open() ) 
      {
        std::cerr << "MapCache::set_up_file_cache_path: Failed to open cached.map for reading: " 
          << my_file_cache_path + "cached.map" << std::endl;
        return;
      }
      std::string key;
      int value;
      if( debug_mode ) 
      {
        std::cout << "MapCache::set_up_file_cache_path: Loading previous disk cache contents from "
          << my_file_cache_path + "cached.map" << std::endl;
      }
      while( file >> key >> value ) 
      {
        if( entry_count >= disk_cache_size ) 
        {
          // Cache too small to hold previous contents
          break;
        }
        if( debug_mode ) 
        {
          std::cout << "MapCache::set_up_file_cache_path: Putting key = " << key << " with entry number = " << entry_count 
            << " into disk cache." << std::endl;
        }
        disk_cache.Put( key, value );
        entry_count++;
      }
      file.close();
      // Delete cached.map file after loading contents into memory
      std::remove( ( my_file_cache_path + "cached.map" ).c_str() );
    } 
    else 
    {
      if( debug_mode ) 
      {
        // Debugging line to see that no previous cache was found
        std::cout << "MapCache::set_up_file_cache_path: MapCache: No previous cache found, fresh start (file cache path: " << 
          my_file_cache_path << ")." << std::endl;
      }
    }
    if( debug_mode ) 
    {
      // Debugging line to see file_cache_path
      std::cout << "MapCache::set_up_file_cache_path: Still using file cache path: " << my_file_cache_path << std::endl;
    }
  }

private:

  /** @brief Callback function invoked when an entry is evicted from the RAM cache
   * @param[in] key Unique key identifying the map data
   * @param[in] value_handle Shared pointer to the JSON object representing the map data
   */
  void on_erase_callback_for_ram_cache( const std::string& key, 
      const lru_cache_t<std::string, nlohmann::json>::value_type& value_handle ) 
  {
    if( disk_cache.TryGet( key ).second || entry_count >= disk_cache_size )
    {
      if( debug_mode )
      {
        // Debugging line to see the key being skipped for disk cache
        std::cout << "MapCache::on_erase_callback_for_ram_cache: Disk cache full or map already exists in disk cache, "
        << "skipping put operation." 
        << std::endl;
      }
      return;
    }
    // If the key is not already in disk cache, put it into disk cache and save the file
    disk_cache.Put( key, entry_count );
    if( debug_mode )
    {
      // Debugging line to see the file_cache_path and entryCount
      std::cout << "MapCache::on_erase_callback_for_ram_cache: Saving entry to disk cache at " << my_file_cache_path
        << "cache.entry_" << entry_count << ".json" << std::endl;
    }
    JsonFileHelpers::save( *value_handle, my_file_cache_path + "cache.entry_" + std::to_string( entry_count++ )
      + ".json", "MapCache::on_erase_callback_for_ram_cache" );
  }

  /** @brief Callback function invoked when an entry is evicted from the disk cache
   * @param[in] key Unique key identifying the map data
   * @param[in] value_handle Shared pointer to the integer representing the entry number
   */
  void on_erase_callback_for_disk_cache( const std::string& key, 
    const lru_cache_t<std::string, int>::value_type& value_handle )
  {
    if( on_final_clear )
    {
      // If on_final_clear is true, we want to keep the file and also save the key-value pair
      std::cout << "MapCache::on_erase_callback_for_disk_cache: Keeping cache entry for key: " << key << std::endl;
      std::string filename = my_file_cache_path + "cached.map";
      if( debug_mode )
      {
        // Debugging line to see the file_cache_path
        std::cout << "MapCache::on_erase_callback_for_disk_cache: file_cache_path: " << my_file_cache_path << std::endl;
      }
      std::ofstream file(filename, std::ios::app);
      if( file.is_open() )
      {
        file << key << " " << *value_handle << "\n"; // Save the key-value pair
        file.close();
      } 
      else
      {
        std::cerr << "MapCache::on_erase_callback_for_disk_cache: Failed to open file for saving cache entry: " 
          << filename << std::endl;
      }
    }
    else
    {
      // If on_final_clear is false, remove the file from disk
      if( debug_mode )
      {
        // Debugging line to see the key being erased from disk cache
        std::cout << "MapCache::on_erase_callback_for_disk_cache: Erasing cache entry for key: " << key << std::endl;
        std::cout << "MapCache::on_erase_callback_for_disk_cache: Removing entry from disk cache at " 
          << my_file_cache_path << "cache.entry_" << *value_handle << ".json" << std::endl;
      }
      std::remove( ( my_file_cache_path + "cache.entry_" + std::to_string( *value_handle ) + ".json" ).c_str() );
      entry_count--;
    }
  }

  std::string my_file_cache_path;
  const size_t ram_cache_size;
  const size_t disk_cache_size;
  std::size_t entry_count;
  bool on_final_clear;
  bool is_active;
  bool debug_mode;
  lru_cache_t<std::string, nlohmann::json> ram_cache;
  lru_xcache_t<std::string, int> disk_cache;
};
