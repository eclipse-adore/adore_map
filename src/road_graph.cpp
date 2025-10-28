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

#include "adore_map/road_graph.hpp"

namespace adore
{
namespace map
{


bool
RoadGraph::add_connection( Connection connection )
{
  to_successors[connection.from_id].insert( connection.to_id );

  to_predecessors[connection.to_id].insert( connection.from_id );

  all_connections.insert( connection );

  return true;
}

std::deque<LaneID>
RoadGraph::find_path( LaneID from, LaneID to, bool allow_reverse ) const
{
  using QueueEntry = std::pair<double, LaneID>;
  std::priority_queue<QueueEntry, std::vector<QueueEntry>, std::greater<>> pq;

  std::unordered_map<LaneID, double> shortest_paths;
  std::unordered_map<LaneID, LaneID> previous_roads;
  std::unordered_set<LaneID>         visited;

  pq.push( { 0.0, from } );
  shortest_paths[from] = 0.0;

  while( !pq.empty() )
  {
    auto [current_cost, current_road] = pq.top();
    pq.pop();

    if( visited.find( current_road ) != visited.end() )
      continue;
    visited.insert( current_road );

    if( current_road == to )
      return reconstruct_path( from, to, previous_roads );

    // Explore both successors and (optionally) predecessors
    auto try_neighbors = [&]( const std::unordered_map<LaneID, std::unordered_set<LaneID>>& neighbor_map, bool reverse_direction ) {
      if( neighbor_map.count( current_road ) == 0 )
        return;

      for( const auto& neighbor : neighbor_map.at( current_road ) )
      {
        std::optional<Connection> conn;
        if( !reverse_direction )
          conn = find_connection( current_road, neighbor );
        else
          conn = find_connection( neighbor, current_road );

        if( !conn )
          continue;

        double new_cost = current_cost + conn->weight;
        if( shortest_paths.find( neighbor ) == shortest_paths.end() || new_cost < shortest_paths[neighbor] )
        {
          shortest_paths[neighbor] = new_cost;
          previous_roads[neighbor] = current_road;
          pq.push( { new_cost, neighbor } );
        }
      }
    };

    try_neighbors( to_successors, false ); // forward traversal
    if( allow_reverse )
      try_neighbors( to_predecessors, true ); // backward traversal
  }

  std::cerr << "failed to find route to end" << std::endl;
  return {};
}

std::deque<LaneID>
RoadGraph::get_best_path( LaneID from, LaneID to ) const
{
  return find_path( from, to, false );
}

std::deque<LaneID>
RoadGraph::reconstruct_path( LaneID from, LaneID to, const std::unordered_map<LaneID, LaneID>& previous_roads ) const
{
  std::deque<LaneID> path;
  LaneID             current = to;

  while( !( current == from ) )
  {
    path.push_front( current );
    current = previous_roads.at( current );
  }
  path.push_front( from );

  return path;
}

std::optional<Connection>
RoadGraph::find_connection( LaneID from_id, LaneID to_id ) const
{
  Connection query;
  query.from_id = from_id;
  query.to_id   = to_id;
  auto it       = all_connections.find( query );
  if( it != all_connections.end() )
  {
    return *it;
  }
  return std::nullopt;
}

} // namespace map
} // namespace adore
