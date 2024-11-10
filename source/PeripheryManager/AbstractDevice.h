#ifndef PERIPHERY_MANAGER_ABSTRACTDEVICE_H
#define PERIPHERY_MANAGER_ABSTRACTDEVICE_H

#include <memory>
#include <utility>
#include <future>
#include <optional>
#include "PeripheryManager/HwInterface.h"
#include "PeripheryManager/ProtocolInterface.h"

class AbstractDevice {
protected:
    explicit AbstractDevice(std::shared_ptr<HwInterface> communication_interface,
                            std::shared_ptr<ProtocolInterface> protocol_interface) :
                               communication_interface_(std::move(communication_interface)),
                               protocol_interface_(std::move(protocol_interface)) {};
    std::optional<std::vector<uint8_t>> getDataSyncronously(const std::vector<uint8_t> &tx_data) const;
    std::future<std::vector<uint8_t>> getDataAsynchronously(const std::vector<uint8_t> &tx_data) const;

public:
    bool init();
    bool deinit();
    virtual ~AbstractDevice() = default;

private:
    std::vector<uint8_t> readData() const;
    bool writeData(const std::vector<uint8_t> &tx_data) const;
    bool isEnabled() const;
    std::shared_ptr<HwInterface> communication_interface_;
    std::shared_ptr<ProtocolInterface> protocol_interface_;
    bool is_enabled_{false};
    virtual bool enable() = 0 ;
    virtual bool disable() = 0;
};

#endif //PERIPHERY_MANAGER_ABSTRACTDEVICE_H
