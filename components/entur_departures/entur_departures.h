#pragma once
#include "esphome/core/component.h"
#include "esphome/components/smart_display/plugin_api.h"

namespace esphome::entur_departures {

// The plugin: one tile type, "next" (plugin:entur_departures.next in a layout).
class EnturDepartures : public Component, public tessera::Plugin {
 public:
  void setup() override;
  float get_setup_priority() const override { return setup_priority::DATA; }
};

}  // namespace esphome::entur_departures
