#ifndef PERIPHERY_MANAGER_HWINTERFACE_H
#define PERIPHERY_MANAGER_HWINTERFACE_H

#include <vector>
#include <cstdint>

class HwInterface {
public:
    virtual ~HwInterface() = default;
    virtual std::vector<uint8_t> read() = 0;
    virtual size_t write(const std::vector<uint8_t>& tx_data) = 0;
    virtual bool init() = 0;
    virtual bool deinit() = 0;
};

class HwFake : public HwInterface {
public:
    HwFake() = default;
    ~HwFake() override = default;
    std::vector<uint8_t> read() override {
        return last_tx_data_;
    }

    size_t write(const std::vector<uint8_t>& tx_data) override {
        last_tx_data_ = tx_data;

        return tx_data.size();
    }

    bool init() override {
        return true;
    }

    bool deinit() override {
        return true;
    }

private:
    std::vector<uint8_t> last_tx_data_;
};

#endif //PERIPHERY_MANAGER_HWINTERFACE_H
