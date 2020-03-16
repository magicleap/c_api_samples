// %BANNER_BEGIN%
// ---------------------------------------------------------------------
// %COPYRIGHT_BEGIN%
//
// Copyright (c) 2018 Magic Leap, Inc. All Rights Reserved.
// Use of this file is governed by the Creator Agreement, located
// here: https://id.magicleap.com/creator-terms
//
// %COPYRIGHT_END%
// ---------------------------------------------------------------------
// %BANNER_END%
#include <app_framework/application.h>
#include <app_framework/convert.h>
#include <app_framework/gui.h>
#include <app_framework/ml_macros.h>
#include <app_framework/toolset.h>

#include <ml_bluetooth.h>
#include <ml_bluetooth_adapter.h>
#include <ml_bluetooth_le.h>
#include <ml_bluetooth_gatt.h>

#include <ml_privileges.h>

#include <chrono>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <vector>

using namespace std::chrono;

struct btdevicecomp {
  bool operator() (MLBluetoothDevice lhs, MLBluetoothDevice rhs) const{
    if (strcmp(lhs.bd_addr.address, rhs.bd_addr.address) <= 0) {
      return true;
    } else {
      return false;
    }
  }
};

constexpr int kguiDevicesMax = 50;

class BluetoothApp : public ml::app_framework::Application {
public:
  BluetoothApp(int argc = 0, char **argv = nullptr) : ml::app_framework::Application(argc, argv) {}
  ~BluetoothApp() = default;

  void OnStart() override {
    RequestPrivileges();

    ml::app_framework::Gui::GetInstance().Initialize(GetGraphicsContext());
    GetRoot()->AddChild(ml::app_framework::Gui::GetInstance().GetNode());

    RegisterAdapterCallback();
    RegisterLeCallback();
    RegisterGattCallback();

    MLBluetoothAdapterState state;
    MLBluetoothAdapterGetState(&state);
    if (state == MLBluetoothAdapterState_Off) {
      ML_LOG(Info, "Turn on Bluetooth! Exiting.");
      StopApp();
      return;
    }

    MLBluetoothName bdname;
    MLBluetoothAdapterGetName(&bdname);
    ML_LOG(Info, "--Local Adapter Information--");
    ML_LOG(Info, "Name: %s, state:%d", bdname.name, state);
    t_scan_ = std::chrono::steady_clock::now();
    MLBluetoothLeStartScan();
  }

  void OnStop() override {
    ml::app_framework::Gui::GetInstance().Cleanup();
    MLPrivilegesShutdown();
  }

  void OnUpdate(float) override {
    std::unique_lock<std::mutex> l(d_mtx_, std::try_to_lock);
    if (l.owns_lock()) {
      UpdateGui();
    }
  }

private:
  void UpdateGui() {
    ml::app_framework::Gui::GetInstance().BeginUpdate();
    if (ImGui::Begin("Bluetooth")) {
      ImGui::Columns(3, "Bluetooth LE Devices");
        ImGui::Separator();
        ImGui::Text("Name"); ImGui::NextColumn();
        ImGui::Text("Address"); ImGui::NextColumn();
        ImGui::Text("RSSI"); ImGui::NextColumn();
        ImGui::Separator();
        static int selected = -1;
        for (int i = 0; i < gui_devices_.size(); i++)
        {
          if (ImGui::Selectable(gui_devices_[i], selected == i, ImGuiSelectableFlags_SpanAllColumns)) {
            selected = i;
          }
          ImGui::NextColumn();
          ImGui::Text("%s", gui_addresses_[i]); ImGui::NextColumn();
          sprintf(tmp, "%d", gui_rssi_[i]);
          ImGui::Text("%s", tmp); ImGui::NextColumn();
        }
        ImGui::Separator();

        if (ImGui::Button("Bond")) {
          if (selected != -1) {
            for (const auto &device : led_devices_) {
              if (strcmp(device.bd_addr.address, gui_addresses_[selected]) == 0) {
                ML_LOG(Info, "Trying to bond with %s:%s", gui_addresses_[selected], gui_devices_[selected]);
                MLBluetoothAdapterCreateBond(&device.bd_addr);
              }
            }
          }
        }
        ImGui::Text("Bonded Status: %s", gui_bonded_status_.c_str());
    }
    ImGui::End();
    ml::app_framework::Gui::GetInstance().EndUpdate();
  }

  void RequestPrivileges() {
    UNWRAP_MLRESULT(MLPrivilegesStartup());
    UNWRAP_MLPRIVILEGES_RESULT_FATAL(MLPrivilegesRequestPrivilege(MLPrivilegeID_BluetoothAdapterUser));
    UNWRAP_MLPRIVILEGES_RESULT_FATAL(MLPrivilegesRequestPrivilege(MLPrivilegeID_BluetoothAdapterExternalApp));
    UNWRAP_MLPRIVILEGES_RESULT_FATAL(MLPrivilegesRequestPrivilege(MLPrivilegeID_BluetoothGattWrite));
  }

  void RegisterAdapterCallback() {
    ML_LOG(Info, "Register adapter callback");
    adapter_callback_.on_adapter_state_changed =
        [](MLBluetoothAdapterState state, void * context) {
      if (state == MLBluetoothAdapterState_On) {
        ML_LOG(Info,
               "Bluetooth adapter is enabled, start to scan LE device...");
        MLBluetoothLeStartScan();
      } else {
        ML_LOG(Info, "Bluetooth adapter is disabled");
      }
    };

    adapter_callback_.on_bond_state_changed =
        [](const MLBluetoothDevice *device, MLBluetoothBondState state,
           void *context) {
      BluetoothApp *app = static_cast<BluetoothApp*>(context);
      switch (state) {
        case MLBluetoothBondState_None:
          ML_LOG(Info, "Not bonded");
          app->gui_bonded_status_ = "Not bonded";
          break;
        case MLBluetoothBondState_Bonding:
          ML_LOG(Info, "Bonding with %s", device->bd_addr.address);
          app->gui_bonded_status_ = std::string("Bonding with ");
          app->gui_bonded_status_ += std::string(device->bd_addr.address);
          break;
        case MLBluetoothBondState_Bonded:
          ML_LOG(Info, "Bonded with %s", device->bd_addr.address);
          ML_LOG(Info, "Trying to connect to GATT");
          app->gui_bonded_status_ = std::string("Bonded with ");
          app->gui_bonded_status_ += std::string(device->bd_addr.address);
          app->gui_bonded_status_ += std::string(" Trying to connect to GATT");
          MLBluetoothGattConnect(&device->bd_addr);
          break;
        default:
          ML_LOG(Info, "Unhandled state");
          break;
      }
    };

    adapter_callback_.on_acl_state_changed =
        [](const MLBluetoothDevice *device, MLBluetoothAclState state,
           void * context) {

      ML_LOG(Info, "Acl state changed, address: %s, state: %d",
             device->bd_addr.address, state);

      if (state == MLBluetoothAclState_Connected) {
        ML_LOG(Info, "ACL is connected.");
      }
    };

    MLBluetoothAdapterSetCallbacks(&adapter_callback_, this);
  }

  void RegisterLeCallback() {
    //
    // With this being callback based we need a way to throttle
    // results.  By having results coming in on another thread it
    // blocks the main thread.  And depending on the results we don't
    // need to get results that fast but we always have to show the
    // gui to make it responsive.  So a user has to write timing code
    // to only process a result when we want but it doesn't stop the
    // thread from taking over the processor and switching out the
    // main thread.
    //
    // This is the crux of the problem with not having a polling based
    // API.  You won't have a context switch because it's on the same
    // thread and can more easily manage what you are checking and
    // processing when.
    //
    scanner_callback_.on_scan_result =
        [](MLBluetoothLeScanResult *result, void * context) {
      if ((result->device.device_type == MLBluetoothDeviceType_LE) && (strlen(result->device.bd_name.name) > 0)) {
        // context is null since it's not available to be filled in with the SetCallbacks function
        BluetoothApp *app = static_cast<BluetoothApp*>(context);
        auto t2 = std::chrono::steady_clock::now();
        auto dscan = t2 - app->t_scan_;
        if (dscan > std::chrono::milliseconds(1000)) {
          ML_LOG(Info, "delta scan: %lld", std::chrono::duration_cast<std::chrono::milliseconds>(dscan).count());
          app->t_scan_ = t2;
          std::unique_lock<std::mutex> l(app->d_mtx_);
          auto it = app->led_devices_.insert(result->device);
          if (it.second && app->gui_devices_.size() < kguiDevicesMax) {
            app->gui_devices_.push_back(&it.first->bd_name.name[0]);
            app->gui_addresses_.push_back(&it.first->bd_addr.address[0]);
            app->gui_rssi_.push_back(it.first->rssi);
            ML_LOG(Info, "inserted address: %s name: %s rssi: %i", result->device.bd_addr.address, result->device.bd_name.name, result->device.rssi);
          }
        }
      }
    };
    MLBluetoothLeSetScannerCallbacks(&scanner_callback_, this);
  }

  void RegisterGattCallback() {
    ML_LOG(Info, "Register GATT callback");
    gatt_client_callback_.on_gatt_connection_state_changed =
        [](MLBluetoothGattStatus status,
           MLBluetoothGattConnectionState new_state, void * context) {

      ML_LOG(Info, "Connection state changed, state: %d, new state: %d", status,
             new_state);

      if (new_state == MLBluetoothGattConnectionState_Connected) {
        ML_LOG(Info, "GATT is connected. Read RSSI value");
        if (MLBluetoothGattReadRemoteRssi() != MLResult_Ok) {
          ML_LOG(Info, "Failed to read remote rssi");
        }

        ML_LOG(Info, "Starting to dicover GATT services");
        if (MLBluetoothGattDiscoverServices() != MLResult_Ok) {
          ML_LOG(Info, "Failed to discover GATT services");
        }
      }
    };

    gatt_client_callback_.on_gatt_services_discovered =
        [](MLBluetoothGattStatus status, void *context) {

      ML_LOG(Info, "Completed discovering Gatt services, status: %d", status);

      if (status != MLBluetoothGattStatus_Success) {
        ML_LOG(Info, "GATT error code: %d", status);
        return;
      }

      MLBluetoothGattServiceList gatt_service_list;
      MLBluetoothGattServiceListInit(&gatt_service_list);
      MLBluetoothGattGetServiceRecord(&gatt_service_list);

      if (!gatt_service_list.services) {
        ML_LOG(Info, "returned service list is null");
        return;
      }

      for (int i = 0; i < gatt_service_list.count; i++) {
        ML_LOG(Info, "Service: %s", gatt_service_list.services[i].uuid.uuid);
        MLBluetoothGattCharacteristicNode *charac_node =
            gatt_service_list.services[i].characteristics;
        while (charac_node != nullptr) {
          ML_LOG(Info, "\tCharacteristic: %s, properties: %d",
                 charac_node->characteristic.uuid.uuid,
                 charac_node->characteristic.properties);

          MLBluetoothGattDescriptorNode *desc =
              charac_node->characteristic.descriptors;
          while (desc != nullptr) {
            ML_LOG(Info, "\t\tDescriptor: %s", desc->descriptor.uuid.uuid);
            desc = desc->next;
          }
          charac_node = charac_node->next;
        }
      }

      ML_LOG(Info, "Done getting all of Gatt services");
      MLBluetoothGattReleaseServiceList(&gatt_service_list);
      ML_LOG(Info, "Updating connection interval");
      MLBluetoothGattRequestConnectionPriority(
                                MLBluetoothGattConnectionPriority_Balanced);
    };

    gatt_client_callback_.on_gatt_read_remote_rssi =
        [](int32_t rssi, MLBluetoothGattStatus status, void *context) {

      ML_LOG(Info, "RSSI value: %d, status: %d", rssi, status);
    };

    gatt_client_callback_.on_gatt_characteristic_read =
        [](const MLBluetoothGattCharacteristic *characteristic,
           MLBluetoothGattStatus status, void *context) {

      ML_LOG(Info, "Read a value for characteristic");
      ML_LOG(Info, "Status: %d", status);
      ML_LOG(Info, "UUID: %s", characteristic->uuid.uuid);
      ML_LOG(Info, "Size of value: %d", characteristic->size);

      for (int i = 0; i < characteristic->size; i++) {
        ML_LOG(Info, "value: %d", characteristic->value[i]);
      }
    };

    gatt_client_callback_.on_gatt_characteristic_write =
        [](const MLBluetoothGattCharacteristic *characteristic,
           MLBluetoothGattStatus status, void *context) {

      ML_LOG(Info, "Wrote a value for characteristic, id: %d, status: %d",
             characteristic->instance_id, status);
    };

    gatt_client_callback_.on_gatt_descriptor_read =
        [](const MLBluetoothGattDescriptor *descriptor, MLBluetoothGattStatus status,
           void * context) {

      ML_LOG(Info, "Read a value for descriptor");
      ML_LOG(Info, "Status: %d", status);
      ML_LOG(Info, "UUID: %s", descriptor->uuid.uuid);
      ML_LOG(Info, "Size of value: %d", descriptor->size);

      for (int i = 0; i < descriptor->size; i++) {
        ML_LOG(Info, "value: %d", descriptor->value[i]);
      }
    };

    gatt_client_callback_.on_gatt_descriptor_write =
        [](const MLBluetoothGattDescriptor *descriptor, MLBluetoothGattStatus status,
           void *context) {

      ML_LOG(Info, "Wrote a value for descriptor, id: %d, status: %d",
             descriptor->instance_id, status);

    };

    gatt_client_callback_.on_gatt_notify =
        [](const MLBluetoothGattCharacteristic *characteristic, void *context) {

      ML_LOG(Info, "onNotify, id: %d", characteristic->instance_id);
      ML_LOG(Info, "Size of value: %d", characteristic->size);

      for (int i = 0; i < characteristic->size; i++) {
        ML_LOG(Info, "Value : %d", characteristic->value[i]);
      }
    };

    gatt_client_callback_.on_gatt_connection_parameters_updated =
        [](int32_t interval, MLBluetoothGattStatus status, void *context) {
      ML_LOG(Info, "Interval updated: %d Status %d", interval, status);
    };

    MLBluetoothGattSetClientCallbacks(&gatt_client_callback_, this);
  }

  MLBluetoothAdapterCallbacks adapter_callback_ = {};
  MLBluetoothLeScannerCallbacks scanner_callback_ = {};
  MLBluetoothLeGattClientCallbacks gatt_client_callback_ = {};
  char tmp[10]; // for itoa during update gui
  std::chrono::time_point<steady_clock> t_scan_;
  std::set<MLBluetoothDevice, btdevicecomp> led_devices_;
  std::mutex d_mtx_;

  std::vector<const char*> gui_devices_;
  std::vector<const char*> gui_addresses_;
  std::vector<uint8_t> gui_rssi_;
  std::string gui_bonded_status_;
};

int main(int argc, char **argv) {
  BluetoothApp app(argc, argv);
  app.SetUseGfx(true);
  app.RunApp();
  return 0;
}
