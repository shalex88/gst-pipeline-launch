#ifndef PERIPHERY_MANAGER_PROTOCOLINTERFACE_H
#define PERIPHERY_MANAGER_PROTOCOLINTERFACE_H

#include <vector>
#include <cstdint>

class ProtocolInterface {
public:
    virtual ~ProtocolInterface() = default;
    virtual std::vector<uint8_t> packData(const std::vector<uint8_t>& tx_data) const = 0;
    virtual std::vector<uint8_t> unpackData(const std::vector<uint8_t>& rx_packet) const = 0;
};

class ProtocolFake : public ProtocolInterface {
public:
    ~ProtocolFake() override = default;
    std::vector<uint8_t> packData(const std::vector<uint8_t>& tx_data) const override {
        return tx_data;
    }

    std::vector<uint8_t> unpackData(const std::vector<uint8_t>& rx_packet) const override {
        return rx_packet;
    }
};

#endif //PERIPHERY_MANAGER_PROTOCOLINTERFACE_H
