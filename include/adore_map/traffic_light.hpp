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
#include <vector>

#include "adore_math/point.h"

namespace adore
{
namespace map
{


struct TrafficLight
{
  std::vector<adore::math::Point2d> control_points;

  enum TrafficLightState
  {
    GREEN,
    RED,
    UNKNOWN,
    AMBER
  } state;

  size_t id;
};

using TrafficLights = std::vector<TrafficLight>;

} // namespace map
} // namespace adore
