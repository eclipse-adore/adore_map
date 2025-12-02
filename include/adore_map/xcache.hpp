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
#include <iostream>
#include <caches/cache.hpp>

namespace caches
{
  
/** 
 * @brief XCache is a fixed sized cache that can be used with different policy types (e.g. LRU, FIFO, LFU).
 * @note This class e(x)tends the class fixed_sized_(cache) to provide a custom cache implementation
 * @tparam Key Type of a key (should be hashable)
 * @tparam Value Type of a value stored in the cache
 * @tparam Policy Type of a policy to be used with the cache
 * @tparam HashMap Type of a hashmap to use for cache operations. Should have std::unordered_map
 * compatible interface
 */
template <typename Key, typename Value, template <typename> class Policy = NoCachePolicy,
          typename HashMap = std::unordered_map<Key, WrappedValue<Value>>>
class XCache : public caches::fixed_sized_cache<Key, Value, Policy, HashMap> 
{
public:
  using base_type = caches::fixed_sized_cache<Key, Value, Policy, HashMap>;
  using typename base_type::map_type;
  using typename base_type::value_type;
  using typename base_type::iterator;
  using typename base_type::const_iterator;
  using typename base_type::operation_guard;
  using typename base_type::on_erase_cb;

  /** @brief XCache constructor
   * @throw std::invalid_argument
   * @param[in] max_size Maximum size of the cache
   * @param[in] policy Cache policy to use
   * @param[in] on_erase on_erase_cb function to be called when cache's element get erased
   */
  explicit XCache( size_t max_size, const Policy<Key>& policy = Policy<Key>{},
                   on_erase_cb on_erase = []( const Key &, const value_type & ) {}, 
                   const bool debug = false
                 )
      : base_type( max_size, policy, on_erase ), debug_mode( debug ) {}

  /** @brief XCache destructor
   */
  ~XCache()
  {
    // Debugging line to see the cache being cleared
    if( debug_mode )
    {
      std::cout << "XCache::~XCache: size = " << base_type::Size() << std::endl;
      std::cout << "XCache::~XCache: Cache gets cleared." << std::endl;
    }
    clear();
    if( debug_mode )
    {
      std::cout << "XCache::~XCache: Cache cleared, size now = " << base_type::Size() << std::endl;
    }
  }

  /** @brief Erase all elements in the cache 
   * @details This method erases all elements in the cache and thereby calls the on_erase_callback
   * for each element.
   */
  void clear()
  {
    operation_guard lock{ safe_op };
    for( const_iterator it = base_type::begin(); it != base_type::end(); )
    { 
      if( debug_mode )
      {
        std::cout << "XCache::clear: Erasing element with key: " << it->first << std::endl;
      }
      base_type::Erase( it++ ); 
      if( debug_mode )
      {
        std::cout << "XCache::clear: Erased element." << std::endl;
      }
    }
  }
private: 

  mutable std::mutex safe_op;
  const bool debug_mode;
};

} // Namespace caches
