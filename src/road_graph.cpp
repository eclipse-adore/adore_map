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

std::deque<DirectedLane>
RoadGraph::find_path( LaneID from, LaneID to, bool start_reverse, const std::function<std::optional<Tangent>( LaneID, bool )>& get_tangent,
                      double max_uturn_cos ) const
{
  using QueueEntry = std::pair<double, DirectedLane>;
  std::priority_queue<QueueEntry, std::vector<QueueEntry>, std::greater<>> pq;

  std::unordered_map<DirectedLane, double, DirectedLaneHasher>       shortest_paths;
  std::unordered_map<DirectedLane, DirectedLane, DirectedLaneHasher> previous_roads;
  std::unordered_set<DirectedLane, DirectedLaneHasher>               visited;

  DirectedLane start_state{ from, start_reverse };
  pq.push( { 0.0, start_state } );
  shortest_paths[start_state] = 0.0;

  DirectedLane goal_state{};
  bool         goal_found = false;

  while( !pq.empty() )
  {
    auto [current_cost, current] = pq.top();
    pq.pop();

    if( visited.count( current ) )
      continue;
    visited.insert( current );

    if( current.lane_id == to )
    {
      goal_state = current;
      goal_found = true;
      break;
    }

    if( to_successors.count( current.lane_id ) == 0 )
      continue;

    for( const auto& neighbor : to_successors.at( current.lane_id ) )
    {
      auto conn = find_connection( current.lane_id, neighbor );
      if( !conn )
        continue;

      bool exit_is_start = ( conn->connection_type == START_TO_START || conn->connection_type == START_TO_END );

      // For non-PARALLEL connections, from_id (current.lane_id) can only be
      // exited at the end implied by connection_type. If our current
      // directed state exits at the other end, this edge isn't usable here.
      if( conn->connection_type != PARALLEL && current.reverse != exit_is_start )
        continue;

      bool to_reverse;
      if( conn->connection_type == PARALLEL )
      {
        // Lateral lane change: doesn't flip direction of travel.
        to_reverse = current.reverse;
      }
      else
      {
        bool entry_is_end = ( conn->connection_type == END_TO_END || conn->connection_type == START_TO_END );
        to_reverse        = entry_is_end;
      }

      // Geometric U-turn check using actual tangents at the junction.
      if( get_tangent )
      {
        bool from_at_end = exit_is_start ? false : true;
        bool to_at_end;
        if( conn->connection_type == PARALLEL )
          to_at_end = from_at_end; // adjacent lane change, mirror the exit side
        else
          to_at_end = ( conn->connection_type == END_TO_END || conn->connection_type == START_TO_END );

        auto from_tangent_raw = get_tangent( current.lane_id, from_at_end );
        auto to_tangent_raw   = get_tangent( neighbor, to_at_end );

        if( from_tangent_raw && to_tangent_raw )
        {
          double fsign = current.reverse ? -1.0 : 1.0;
          double tsign = to_reverse ? -1.0 : 1.0;

          double fx = from_tangent_raw->first * fsign;
          double fy = from_tangent_raw->second * fsign;
          double tx = to_tangent_raw->first * tsign;
          double ty = to_tangent_raw->second * tsign;

          double dot = fx * tx + fy * ty; // both are unit vectors, dot = cos(angle)

          if( dot < max_uturn_cos )
            continue; // reject: this transition reverses heading too sharply
        }
      }

      DirectedLane neighbor_state{ neighbor, to_reverse };
      double       new_cost = current_cost + conn->weight;

      if( shortest_paths.find( neighbor_state ) == shortest_paths.end() || new_cost < shortest_paths[neighbor_state] )
      {
        shortest_paths[neighbor_state] = new_cost;
        previous_roads[neighbor_state] = current;
        pq.push( { new_cost, neighbor_state } );
      }
    }
  }

  if( !goal_found )
  {
    std::cerr << "failed to find u-turn-constrained route from " << from << " to " << to << std::endl;
    return {};
  }

  std::deque<DirectedLane> path;
  DirectedLane             current = goal_state;
  while( !( current.lane_id == from && current.reverse == start_reverse ) )
  {
    path.push_front( current );
    current = previous_roads.at( current );
  }
  path.push_front( DirectedLane{ from, start_reverse } );

  return path;
}

// --- get_best_path (5-arg overload) ---

std::deque<DirectedLane>
RoadGraph::get_best_path( LaneID from, LaneID to, bool start_reverse,
                          const std::function<std::optional<Tangent>( LaneID, bool )>& get_tangent, double max_uturn_cos ) const
{
  return find_path( from, to, start_reverse, get_tangent, max_uturn_cos );
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
