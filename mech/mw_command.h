// Copyright 2019-2020 Josh Pieper, jjp@pobox.com.
// Copyright 2015-2016 Mikhail Afanasyev.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include <clipp/clipp.h>

#include "mjlib/base/fail.h"

#include "base/component_archives.h"
#include "base/logging.h"

#include "mech_warfare_command.h"
#include "video_controller_app.h"

namespace mjmech {
namespace mech {

// this is a class for the main mw_command binary
class MWCommand : boost::noncopyable {
 public:
  template <typename Context>
  MWCommand(Context& context)
    : executor_(context.executor) {
    m_.video_controller.reset(new VideoControllerApp(context));
    m_.commander.reset(new mw_command::Commander(executor_));
  }

  void AsyncStart(mjlib::io::ErrorCallback handler) {
    std::shared_ptr<base::ErrorHandlerJoiner> joiner =
        std::make_shared<base::ErrorHandlerJoiner>(
            [this, handler=std::move(handler)](mjlib::base::error_code ec) mutable {
              if (!ec) {
                boost::asio::post(
                    executor_,
                    [this]() {
                      MaybeSendOnce();
                    });
              }
              handler(ec);
            });

    m_.video_controller->AsyncStart(joiner->Wrap("starting video_controller"));
    m_.commander->AsyncStart(joiner->Wrap("starting commander"));

    m_.commander->target_offset_signal()->connect([this](int x, int y) {
        m_.video_controller->SetTargetOffset(x, y);
      });
  }

  struct Members {
    std::unique_ptr<VideoControllerApp> video_controller;
    std::unique_ptr<mw_command::Commander> commander;

    template <typename Archive>
    void Serialize(Archive* a) {
      a->Visit(MJ_NVP(video_controller));
      a->Visit(MJ_NVP(commander));
    }
  };

  struct Parameters {
    // If true, sends a command once and immediately exits.
    bool send_once = false;
    double turret_pitch_rate_dps = 0.0;
    double turret_yaw_rate_dps = 0.0;


    template <typename Archive>
    void Serialize(Archive* a) {
      a->Visit(MJ_NVP(send_once));
      a->Visit(MJ_NVP(turret_pitch_rate_dps));
      a->Visit(MJ_NVP(turret_yaw_rate_dps));
    }
  };

  clipp::group program_options() {
    return (mjlib::base::ClippArchive().Accept(&parameters_).release(),
            base::ClippComponentArchive().Accept(&m_).release());
  }

 private:

  void MaybeSendOnce() {
    bool turret_set = (
        parameters_.turret_pitch_rate_dps != 0.0 ||
        parameters_.turret_yaw_rate_dps != 0.0);

    if (!parameters_.send_once) {
      if (turret_set) {
        mjlib::base::Fail("turret_* options have no effect when send_once=False");
      }
      return;
    }

    mw_command::MechMessage message;
    message.gait = m_.commander->parameters()->cmd;
    TurretCommand& turret = message.turret;
    if (turret_set) {
      turret.rate = TurretCommand::Rate();
      turret.rate->x_deg_s = parameters_.turret_yaw_rate_dps;
      turret.rate->y_deg_s = parameters_.turret_pitch_rate_dps;
    }
    m_.commander->SendMechMessage(message);
    log_.info("message sent, exiting");

    boost::asio::post(
        executor_,
        []() {
          std::exit(0);
        });
  };

  boost::asio::executor executor_;
  Members m_;
  Parameters parameters_;
  base::LogRef log_ = base::GetLogInstance("mw_command");
};

}
}
