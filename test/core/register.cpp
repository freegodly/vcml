/******************************************************************************
 *                                                                            *
 * Copyright 2018 Jan Henrik Weinstock                                        *
 *                                                                            *
 * Licensed under the Apache License, Version 2.0 (the "License");            *
 * you may not use this file except in compliance with the License.           *
 * You may obtain a copy of the License at                                    *
 *                                                                            *
 *     http://www.apache.org/licenses/LICENSE-2.0                             *
 *                                                                            *
 * Unless required by applicable law or agreed to in writing, software        *
 * distributed under the License is distributed on an "AS IS" BASIS,          *
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.   *
 * See the License for the specific language governing permissions and        *
 * limitations under the License.                                             *
 *                                                                            *
 ******************************************************************************/

#include <gtest/gtest.h>
#include <gmock/gmock.h>

using namespace ::testing;

#include "vcml.h"

using ::vcml::u32;

class mock_peripheral: public vcml::peripheral {
public:
    vcml::reg<u32> test_reg_a;
    vcml::reg<u32> test_reg_b;

    MOCK_METHOD(u32, reg_read, ());
    MOCK_METHOD(u32, reg_write, (u32));

    mock_peripheral(const sc_core::sc_module_name& nm =
        sc_core::sc_gen_unique_name("mock_peripheral")):
        vcml::peripheral(nm, vcml::ENDIAN_LITTLE, 1, 10),
        test_reg_a("test_reg_a", 0x0, 0xffffffff),
        test_reg_b("test_reg_b", 0x4, 0xffffffff) {
        test_reg_b.allow_read_write();
        test_reg_b.on_read(&mock_peripheral::reg_read);
        test_reg_b.on_write(&mock_peripheral::reg_write);
        CLOCK.stub(100 * vcml::MHz);
        RESET.stub();
        handle_clock_update(0, CLOCK.read());
    }

    unsigned int test_transport(tlm::tlm_generic_payload& tx) {
        return transport(tx, vcml::SBI_NONE, vcml::VCML_AS_DEFAULT);
    }

};

TEST(registers, read) {
    mock_peripheral mock;
    sc_core::sc_time cycle(1.0 / mock.CLOCK, sc_core::SC_SEC);
    sc_core::sc_time& local = mock.local_time();
    tlm::tlm_generic_payload tx;

    unsigned char buffer [] = { 0xcc, 0xcc, 0xcc, 0xcc };
    unsigned char expect [] = { 0x37, 0x13, 0x00, 0x00 };

    mock.test_reg_a = 0x1337;
    local = sc_core::SC_ZERO_TIME;
    vcml::tx_setup(tx, tlm::TLM_READ_COMMAND, 0, buffer, sizeof(buffer));

    EXPECT_EQ(mock.test_transport(tx), 4);
    EXPECT_EQ(mock.test_reg_a, 0x00001337u);
    EXPECT_EQ(mock.test_reg_b, 0xffffffffu);
    EXPECT_EQ(buffer[0], expect[0]);
    EXPECT_EQ(buffer[1], expect[1]);
    EXPECT_EQ(buffer[2], expect[2]);
    EXPECT_EQ(buffer[3], expect[3]);
    EXPECT_EQ(local, cycle * mock.read_latency);
    EXPECT_TRUE(tx.is_response_ok());
}

TEST(registers, read_callback) {
    mock_peripheral mock;
    sc_core::sc_time cycle(1.0 / mock.CLOCK, sc_core::SC_SEC);
    sc_core::sc_time& local = mock.local_time();
    tlm::tlm_generic_payload tx;

    unsigned char buffer [] = { 0xcc, 0xcc, 0xcc, 0xcc };
    unsigned char expect [] = { 0x37, 0x13, 0x00, 0x00 };

    mock.test_reg_b = 0x1337;
    local = sc_core::SC_ZERO_TIME;
    vcml::tx_setup(tx, tlm::TLM_READ_COMMAND, 4, buffer, sizeof(buffer));

    EXPECT_CALL(mock, reg_read()).WillOnce(Return(mock.test_reg_b.get()));
    EXPECT_EQ(mock.test_transport(tx), 4);
    EXPECT_EQ(mock.test_reg_a, 0xffffffffu);
    EXPECT_EQ(mock.test_reg_b, 0x00001337u);
    EXPECT_EQ(buffer[0], expect[0]);
    EXPECT_EQ(buffer[1], expect[1]);
    EXPECT_EQ(buffer[2], expect[2]);
    EXPECT_EQ(buffer[3], expect[3]);
    EXPECT_EQ(local, cycle * mock.read_latency);
    EXPECT_TRUE(tx.is_response_ok());
}

TEST(registers, write) {
    mock_peripheral mock;
    sc_core::sc_time cycle(1.0 / mock.CLOCK, sc_core::SC_SEC);
    sc_core::sc_time& local = mock.local_time();
    tlm::tlm_generic_payload tx;

    unsigned char buffer [] = { 0x11, 0x22, 0x33, 0x44 };

    local = sc_core::SC_ZERO_TIME;
    vcml::tx_setup(tx, tlm::TLM_WRITE_COMMAND, 0, buffer, sizeof(buffer));

    EXPECT_EQ(mock.test_transport(tx), 4);
    EXPECT_EQ(mock.test_reg_a, 0x44332211u);
    EXPECT_EQ(mock.test_reg_b, 0xffffffffu);
    EXPECT_EQ(local, cycle * mock.write_latency);
    EXPECT_TRUE(tx.is_response_ok());
}

TEST(registers, write_callback) {
    mock_peripheral mock;
    sc_core::sc_time cycle(1.0 / mock.CLOCK, sc_core::SC_SEC);
    sc_core::sc_time& local = mock.local_time();
    tlm::tlm_generic_payload tx;

    u32 value = 0x98765432;
    unsigned char buffer [] = { 0x11, 0x22, 0x33, 0x44 };

    local = sc_core::SC_ZERO_TIME;
    vcml::tx_setup(tx, tlm::TLM_WRITE_COMMAND, 4, buffer, sizeof(buffer));

    EXPECT_CALL(mock, reg_write(0x44332211)).WillOnce(Return(value));
    EXPECT_EQ(mock.test_transport(tx), 4);
    EXPECT_EQ(mock.test_reg_a, 0xffffffff);
    EXPECT_EQ(mock.test_reg_b, value);
    EXPECT_EQ(local, cycle * mock.write_latency);
    EXPECT_TRUE(tx.is_response_ok());
}

TEST(registers, read_byte_enable) {
    mock_peripheral mock;
    sc_core::sc_time cycle(1.0 / mock.CLOCK, sc_core::SC_SEC);
    sc_core::sc_time& local = mock.local_time();
    tlm::tlm_generic_payload tx;

    unsigned char buffer [] = { 0xcc, 0xcc, 0x00, 0x00 };
    unsigned char bebuff [] = { 0xff, 0xff, 0x00, 0x00 };
    unsigned char expect [] = { 0x37, 0x13, 0x00, 0x00 };

    mock.test_reg_a = 0x1337;
    local = sc_core::SC_ZERO_TIME;
    vcml::tx_setup(tx, tlm::TLM_READ_COMMAND, 0, buffer, sizeof(buffer));
    tx.set_byte_enable_ptr(bebuff);
    tx.set_byte_enable_length(sizeof(bebuff));

    EXPECT_EQ(mock.test_transport(tx), 2);
    EXPECT_EQ(mock.test_reg_a, 0x00001337u);
    EXPECT_EQ(mock.test_reg_b, 0xffffffffu);
    EXPECT_EQ(buffer[0], expect[0]);
    EXPECT_EQ(buffer[1], expect[1]);
    EXPECT_EQ(buffer[2], expect[2]);
    EXPECT_EQ(buffer[3], expect[3]);
    EXPECT_EQ(local, cycle * mock.read_latency);
    EXPECT_TRUE(tx.is_response_ok());
}

TEST(registers, write_byte_enable) {
    mock_peripheral mock;
    sc_core::sc_time cycle(1.0 / mock.CLOCK, sc_core::SC_SEC);
    sc_core::sc_time& local = mock.local_time();
    tlm::tlm_generic_payload tx;

    unsigned char buffer [] = { 0x11, 0x22, 0x33, 0x44 };
    unsigned char bebuff [] = { 0xff, 0x00, 0xff, 0x00 };

    mock.test_reg_a = 0;
    local = sc_core::SC_ZERO_TIME;
    vcml::tx_setup(tx, tlm::TLM_WRITE_COMMAND, 0, buffer, sizeof(buffer));
    tx.set_byte_enable_ptr(bebuff);
    tx.set_byte_enable_length(sizeof(bebuff));

    EXPECT_EQ(mock.test_transport(tx), 2);
    EXPECT_EQ(mock.test_reg_a, 0x00330011u);
    EXPECT_EQ(mock.test_reg_b, 0xffffffffu);
    EXPECT_EQ(local, cycle * mock.write_latency);
    EXPECT_TRUE(tx.is_response_ok());
}

TEST(registers, permissions) {
    mock_peripheral mock;

    sc_core::sc_time cycle(1.0 / mock.CLOCK, sc_core::SC_SEC);
    sc_core::sc_time& local = mock.local_time();

    tlm::tlm_generic_payload tx;
    unsigned char buffer [] = { 0x11, 0x22, 0x33, 0x44 };

    local = sc_core::SC_ZERO_TIME;
    mock.test_reg_b.allow_read_only();
    vcml::tx_setup(tx, tlm::TLM_WRITE_COMMAND, 4, buffer, sizeof(buffer));

    EXPECT_CALL(mock, reg_write(_)).Times(0);
    EXPECT_EQ(mock.test_transport(tx), 0);
    EXPECT_EQ(tx.get_response_status(), tlm::TLM_COMMAND_ERROR_RESPONSE);
    EXPECT_EQ(mock.test_reg_a, 0xffffffffu);
    EXPECT_EQ(mock.test_reg_b, 0xffffffffu);
    EXPECT_EQ(local, cycle * mock.write_latency);

    local = sc_core::SC_ZERO_TIME;
    mock.test_reg_b.allow_write_only();
    vcml::tx_setup(tx, tlm::TLM_READ_COMMAND, 4, buffer, sizeof(buffer));

    EXPECT_CALL(mock, reg_read()).Times(0);
    EXPECT_EQ(mock.test_transport(tx), 0);
    EXPECT_EQ(tx.get_response_status(), tlm::TLM_COMMAND_ERROR_RESPONSE);
    EXPECT_EQ(mock.test_reg_a, 0xffffffffu);
    EXPECT_EQ(mock.test_reg_b, 0xffffffffu);
    EXPECT_EQ(local, cycle * mock.read_latency);
}

TEST(registers, misaligned_accesses) {
    mock_peripheral mock;

    sc_core::sc_time cycle(1.0 / mock.CLOCK, sc_core::SC_SEC);
    sc_core::sc_time& local = mock.local_time();

    tlm::tlm_generic_payload tx;
    unsigned char buffer [] = { 0x11, 0x22, 0x33, 0x44 };

    mock.test_reg_a = 0;
    local = sc_core::SC_ZERO_TIME;
    vcml::tx_setup(tx, tlm::TLM_WRITE_COMMAND, 1, buffer, 2);

    EXPECT_EQ(mock.test_transport(tx), 2);
    EXPECT_EQ(mock.test_reg_a, 0x00221100u);
    EXPECT_EQ(mock.test_reg_b, 0xffffffffu);
    EXPECT_EQ(local, cycle * mock.write_latency);
    EXPECT_TRUE(tx.is_response_ok());

    local = sc_core::SC_ZERO_TIME;
    vcml::tx_setup(tx, tlm::TLM_WRITE_COMMAND, 1, buffer, 4);

    EXPECT_CALL(mock, reg_write(0xffffff44)).WillOnce(Return(0xffffff44));
    EXPECT_EQ(mock.test_transport(tx), 4); // !
    EXPECT_EQ(mock.test_reg_a, 0x33221100u);
    EXPECT_EQ(mock.test_reg_b, 0xffffff44u); // !
    EXPECT_EQ(local, cycle * mock.write_latency);
    EXPECT_TRUE(tx.is_response_ok());

    unsigned char largebuf [8] = { 0xff };
    local = sc_core::SC_ZERO_TIME;
    vcml::tx_setup(tx, tlm::TLM_READ_COMMAND, 0, largebuf, 8);

    EXPECT_CALL(mock, reg_read()).WillOnce(Return(mock.test_reg_b.get()));
    EXPECT_EQ(mock.test_transport(tx), 8);
    EXPECT_EQ(largebuf[0], 0x00);
    EXPECT_EQ(largebuf[1], 0x11);
    EXPECT_EQ(largebuf[2], 0x22);
    EXPECT_EQ(largebuf[3], 0x33);
    EXPECT_EQ(largebuf[4], 0x44);
    EXPECT_EQ(largebuf[5], 0xff);
    EXPECT_EQ(largebuf[6], 0xff);
    EXPECT_EQ(largebuf[7], 0xff);
    EXPECT_EQ(local, cycle * mock.read_latency);
    EXPECT_TRUE(tx.is_response_ok());
}

TEST(registers, banking) {
    mock_peripheral mock;
    mock.test_reg_a.set_banked();

    sc_core::sc_time cycle(1.0 / mock.CLOCK, sc_core::SC_SEC);

    tlm::tlm_generic_payload tx;
    vcml::sbiext bank;
    vcml::tlm_sbi bank1, bank2;
    const vcml::u8 val1 = 0xab;
    const vcml::u8 val2 = 0xcd;
    unsigned char buffer;

    bank1.cpuid = 1;
    bank2.cpuid = 2;

    tx.set_extension(&bank);

    buffer = val1;
    bank.cpuid = 1;
    vcml::tx_setup(tx, tlm::TLM_WRITE_COMMAND, 0, &buffer, 1);
    EXPECT_EQ(mock.transport(tx, bank1, vcml::VCML_AS_DEFAULT), 1);
    EXPECT_TRUE(tx.is_response_ok());

    buffer = val2;
    bank.cpuid = 2;
    vcml::tx_setup(tx, tlm::TLM_WRITE_COMMAND, 0, &buffer, 1);
    EXPECT_EQ(mock.transport(tx, bank2, vcml::VCML_AS_DEFAULT), 1);
    EXPECT_TRUE(tx.is_response_ok());

    buffer = 0x0;
    bank.cpuid = 1;
    vcml::tx_setup(tx, tlm::TLM_READ_COMMAND, 0, &buffer, 1);
    EXPECT_EQ(mock.transport(tx, bank1, vcml::VCML_AS_DEFAULT), 1);
    EXPECT_TRUE(tx.is_response_ok());
    EXPECT_EQ(buffer, val1);

    buffer = 0x0;
    bank.cpuid = 2;
    vcml::tx_setup(tx, tlm::TLM_READ_COMMAND, 0, &buffer, 1);
    EXPECT_EQ(mock.transport(tx, bank2, vcml::VCML_AS_DEFAULT), 1);
    EXPECT_TRUE(tx.is_response_ok());
    EXPECT_EQ(buffer, val2);

    tx.clear_extension(&bank);
}

TEST(registers, endianess) {
    mock_peripheral mock;
    mock.set_big_endian();

    sc_core::sc_time cycle(1.0 / mock.CLOCK, sc_core::SC_SEC);
    sc_core::sc_time& local = mock.local_time();

    tlm::tlm_generic_payload tx;
    u32 buffer = 0;

    mock.test_reg_a = 0x11223344;
    vcml::tx_setup(tx, tlm::TLM_READ_COMMAND, 0, &buffer, 4);
    EXPECT_EQ(mock.test_transport(tx), 4);
    EXPECT_EQ(buffer, 0x44332211);
    EXPECT_EQ(local, cycle * mock.read_latency);
    EXPECT_TRUE(tx.is_response_ok());

    buffer = 0xeeff00cc;
    local = sc_core::SC_ZERO_TIME;
    vcml::tx_setup(tx, tlm::TLM_WRITE_COMMAND, 0, &buffer, 4);
    EXPECT_EQ(mock.test_transport(tx), 4);
    EXPECT_EQ(mock.test_reg_a, 0xcc00ffeeu);
    EXPECT_EQ(local, cycle * mock.write_latency);
    EXPECT_TRUE(tx.is_response_ok());
}

TEST(registers, operators) {
    mock_peripheral mock;

    mock.test_reg_a = 3;
    mock.test_reg_b = 3;

    EXPECT_TRUE(mock.test_reg_a == 3u);
    EXPECT_TRUE(mock.test_reg_b == 3u);

    EXPECT_FALSE(mock.test_reg_a != 3u);
    EXPECT_FALSE(mock.test_reg_b != 3u);

    EXPECT_EQ(mock.test_reg_a++, 3u);
    EXPECT_EQ(mock.test_reg_a,   4u);
    EXPECT_EQ(++mock.test_reg_a, 5u);

    EXPECT_EQ(mock.test_reg_b--, 3u);
    EXPECT_EQ(mock.test_reg_b,   2u);
    EXPECT_EQ(--mock.test_reg_b, 1u);

    EXPECT_EQ(mock.test_reg_b += 1, 2u);
    EXPECT_EQ(mock.test_reg_a -= 1, 4u);
}

enum : vcml::address_space {
    VCML_AS_TEST1 = vcml::VCML_AS_DEFAULT + 1,
    VCML_AS_TEST2 = vcml::VCML_AS_DEFAULT + 2,
};

class mock_peripheral_as: public vcml::peripheral {
public:
    vcml::reg<u32> test_reg_a;
    vcml::reg<u32> test_reg_b;

    mock_peripheral_as(const sc_core::sc_module_name& nm =
        sc_core::sc_gen_unique_name("mock_peripheral_as")):
        vcml::peripheral(nm, vcml::ENDIAN_LITTLE, 1, 10),
        test_reg_a(VCML_AS_TEST1, "test_reg_a", 0x0, 0xffffffff),
        test_reg_b(VCML_AS_TEST2, "test_reg_b", 0x0, 0xffffffff) {
        test_reg_b.allow_read_write();
        test_reg_b.allow_read_write();
        CLOCK.stub(100 * vcml::MHz);
        RESET.stub();
        handle_clock_update(0, CLOCK.read());
    }

    unsigned int test_transport(tlm::tlm_generic_payload& tx,
        vcml::address_space as) {
        return transport(tx, vcml::SBI_NONE, as);
    }

};

TEST(registers, address_spaces) {
    // reg_a and reg_b both live at 0x0, but in different address spaces
    mock_peripheral_as mock;

    tlm::tlm_generic_payload tx;
    unsigned char buffer [] = { 0x11, 0x22, 0x33, 0x44 };
    vcml::tx_setup(tx, tlm::TLM_WRITE_COMMAND, 0, buffer, sizeof(buffer));

    // writes to default address space should get lost in the void
    EXPECT_EQ(mock.test_transport(tx, vcml::VCML_AS_DEFAULT), 0);
    EXPECT_EQ(mock.test_reg_a, 0xffffffffu);
    EXPECT_EQ(mock.test_reg_b, 0xffffffffu);
    EXPECT_EQ(tx.get_response_status(), tlm::TLM_ADDRESS_ERROR_RESPONSE);
    mock.reset();

    // writes to VCML_AS_TEST1 should only change reg_a
    EXPECT_EQ(mock.test_transport(tx, VCML_AS_TEST1), 4);
    EXPECT_EQ(mock.test_reg_a, 0x44332211u);
    EXPECT_EQ(mock.test_reg_b, 0xffffffffu);
    EXPECT_TRUE(tx.is_response_ok());
    mock.reset();

    // writes to VCML_AS_TEST2 should only change reg_b
    EXPECT_EQ(mock.test_transport(tx, VCML_AS_TEST2), 4);
    EXPECT_EQ(mock.test_reg_a, 0xffffffffu);
    EXPECT_EQ(mock.test_reg_b, 0x44332211u);
    EXPECT_TRUE(tx.is_response_ok());
    mock.reset();
}

class lambda_test : public vcml::peripheral
{
public:
    vcml::reg<u32> REG;
    lambda_test(const sc_core::sc_module_name& nm):
        vcml::peripheral(nm),
        REG("REG", 0) {
        REG.allow_read_only();
        REG.on_read([&]() -> vcml::u32 {
            return 0x42;
        });
    };

    virtual ~lambda_test() = default;
};

TEST(registers, lambda) {
    lambda_test test("lambda");

    u32 data = 0;
    tlm::tlm_generic_payload tx;
    vcml::tx_setup(tx, tlm::TLM_READ_COMMAND, 0, &data, sizeof(data));
    test.transport(tx, vcml::SBI_NONE, vcml::VCML_AS_DEFAULT);
    EXPECT_TRUE(tx.is_response_ok());
    EXPECT_EQ(data, 0x42);
}

class hierarchy_test : public vcml::peripheral
{
public:
    class wrapper : public sc_core::sc_module
    {
    public:
        vcml::reg<vcml::u64> TEST_REG;

        wrapper(const sc_core::sc_module_name& nm):
            sc_core::sc_module(nm), TEST_REG("TEST_REG", 0) {
        }

        virtual ~wrapper() = default;
    };

    wrapper W;

    hierarchy_test(const sc_core::sc_module_name& nm):
        vcml::peripheral(nm),
        W("W") {
    }
};

TEST(registers, hierarchy) {
    hierarchy_test H("H");
    EXPECT_STREQ(H.W.TEST_REG.name(), "H.W.TEST_REG");
    std::vector<vcml::reg_base*> regs = H.get_registers();
    ASSERT_FALSE(regs.empty());
    EXPECT_STREQ(regs[0]->name(), "H.W.TEST_REG");
    EXPECT_EQ(regs[0], (vcml::reg_base*)&H.W.TEST_REG);
}
