/******************************************************************************
 * Copyright 2017 The Apollo Authors. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *****************************************************************************/

/**
 * @file planning_data.cc
 **/

#include "modules/planning/common/planning_data.h"

#include <utility>

#include "modules/planning/common/path/path_data.h"
#include "modules/planning/proto/planning.pb.h"

namespace apollo {
namespace planning {

const ReferenceLine& PlanningData::reference_line() const {
  return *reference_line_.get();
}

const DecisionData& PlanningData::decision_data() const {
  return *decision_data_.get();
}

const TrajectoryPoint& PlanningData::init_planning_point() const {
  return init_planning_point_;
}

DecisionData* PlanningData::mutable_decision_data() const {
  return decision_data_.get();
}

void PlanningData::set_init_planning_point(
    const TrajectoryPoint& init_planning_point) {
  init_planning_point_ = init_planning_point;
}

void PlanningData::set_reference_line(
    const std::vector<ReferencePoint>& ref_points) {
  reference_line_.reset(new ReferenceLine(ref_points));
}

void PlanningData::set_decision_data(
    std::shared_ptr<DecisionData>& decision_data) {
  decision_data_ = decision_data;
}

const PathData& PlanningData::path_data() const { return path_data_; }

const SpeedData& PlanningData::speed_data() const { return speed_data_; }

PathData* PlanningData::mutable_path_data() { return &path_data_; }

SpeedData* PlanningData::mutable_speed_data() { return &speed_data_; }

bool PlanningData::aggregate(const double time_resolution,
                             PublishableTrajectory* trajectory) {
  if (time_resolution < 0.0) {
    AERROR << "time_resolution: " << time_resolution << " < 0.0";
    return false;
  }
  if (!trajectory) {
    AERROR << "The input trajectory is empty";
    return false;
  }

  for (double cur_rel_time = 0.0; cur_rel_time < speed_data_.total_time();
       cur_rel_time += time_resolution) {
    SpeedPoint speed_point;
    QUIT_IF(!speed_data_.get_speed_point_with_time(cur_rel_time, &speed_point),
            false, ERROR, "Fail to get speed point with relative time %f",
            cur_rel_time);

    common::PathPoint path_point;
    // TODO(all): temp fix speed point s out of path point bound, need further
    // refine later
    if (speed_point.s() > path_data_.path().param_length()) {
      break;
    }
    QUIT_IF(
        !path_data_.get_path_point_with_path_s(speed_point.s(), &path_point),
        false, ERROR, "Fail to get path data with s %f, path total length %f",
        speed_point.s(), path_data_.path().param_length());

    common::TrajectoryPoint trajectory_point;
    trajectory_point.mutable_path_point()->CopyFrom(path_point);
    trajectory_point.set_v(speed_point.v());
    trajectory_point.set_a(speed_point.a());
    trajectory_point.set_relative_time(init_planning_point_.relative_time() +
                                       speed_point.t());
    trajectory->add_trajectory_point(trajectory_point);
  }
  return true;
}

std::string PlanningData::DebugString() const {
  std::ostringstream ss;
  ss << "path_data:" << path_data_.DebugString();
  ss << "speed_data:" << speed_data_.DebugString();
  return ss.str();
}

}  // namespace planning
}  // namespace apollo
