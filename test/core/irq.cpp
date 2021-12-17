/******************************************************************************
 *                                                                            *
 * Copyright 2021 Jan Henrik Weinstock                                        *
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

#include "testing.h"

TEST(irq, to_string) {
    irq_payload irq;
    irq.vector = 42;
    irq.active = true;

    // no formatting checks, just make sure it compiles
    std::cout << irq << std::endl;
}


class irq_test_harness : public test_base
{
public:
    unsigned int irq_no;
    unordered_map<irq_vector, bool> irq_state;
    unordered_set<unsigned int> irq_source;

    irq_initiator_socket OUT;
    irq_initiator_socket OUT2;
    irq_target_socket_array<> IN;

    // for testing hierarchical binding
    irq_base_initiator_socket H_OUT;
    irq_base_target_socket H_IN;

    // for adapter testing
    irq_initiator_socket A_OUT;
    irq_target_adapter TA;
    sc_signal<bool> signal;
    irq_initiator_adapter IA;

    irq_test_harness(const sc_module_name& nm):
        test_base(nm),
        irq_no(),
        irq_state(),
        irq_source(),
        OUT("OUT"),
        OUT2("OUT2"),
        IN("IN"),
        H_OUT("H_OUT"),
        H_IN("H_IN"),
        A_OUT("A_OUT"),
        TA("TA"),
        signal("signal"),
        IA("IA") {
        OUT.bind(IN[0]);

        // check hierarchical binding: OUT -> H_OUT -> H_IN -> IN[1]
        OUT.bind(H_OUT);
        H_IN.bind(IN[1]);
        H_OUT.bind(H_IN);

        // check stubbing
        OUT2.stub();
        IN[2].stub();

        // check adapters
        A_OUT.bind(TA.IRQ_IN);
        TA.IRQ_OUT.bind(signal);
        IA.IRQ_IN.bind(signal);
        IA.IRQ_OUT.bind(IN[3]);

        auto initiators = get_irq_initiator_sockets();
        auto targets = get_irq_target_sockets();
        auto sockets = get_irq_target_sockets(0);

        EXPECT_EQ(initiators.size(), 3) << "irq initiators did not register";
        EXPECT_EQ(targets.size(), 4) << "irq targets did not register";
        EXPECT_FALSE(sockets.empty()) << "irq targets in wrong address space";

        CLOCK.stub(100 * MHz);
        RESET.stub();
    }

    virtual void irq_transport(const irq_target_socket& socket,
        irq_payload& irq) override {
        irq_state[irq.vector] = irq.active;
        size_t source = IN.index_of(socket);
        if (irq.active)
            irq_source.insert(source);
        else
            irq_source.erase(source);
    }

    virtual void run_test() override {
        // this also forces construction of IN[0]'s default event so that it
        // it can be triggered later on
        EXPECT_TRUE(IN[0].default_event().name());

        const irq_vector VECTOR = 0x42;

        EXPECT_FALSE(irq_state[VECTOR]);
        OUT[VECTOR] = true;
        EXPECT_TRUE(irq_state[VECTOR]);
        EXPECT_TRUE(irq_source.count(0));
        EXPECT_TRUE(irq_source.count(1));

        wait(IN[0].default_event());

        EXPECT_TRUE(irq_state[VECTOR]);
        OUT[VECTOR] = false;
        EXPECT_FALSE(irq_state[VECTOR]);
        EXPECT_FALSE(irq_source.count(0));
        EXPECT_FALSE(irq_source.count(1));

        wait(IN[0].default_event());

        EXPECT_FALSE(irq_state[IRQ_NO_VECTOR]);
        OUT = true;
        EXPECT_TRUE(irq_state[IRQ_NO_VECTOR]);
        EXPECT_TRUE(irq_source.count(0));
        EXPECT_TRUE(irq_source.count(1));

        wait(IN[0].default_event());

        EXPECT_TRUE(irq_state[IRQ_NO_VECTOR]);
        OUT = false;
        EXPECT_FALSE(irq_state[IRQ_NO_VECTOR]);
        EXPECT_FALSE(irq_source.count(0));
        EXPECT_FALSE(irq_source.count(1));

        // test hierarchy binding
        EXPECT_FALSE(signal.read());
        A_OUT = true;
        wait(IN[3].default_event());
        EXPECT_TRUE(IN[3].read());
        A_OUT = false;
        wait(IN[3].default_event());
        EXPECT_FALSE(IN[3].read());
    }
};

TEST(irq, sockets) {
    broker_arg broker(sc_argc(), sc_argv());
    log_term logger;
    logger.set_level(LOG_TRACE);
    irq_test_harness test("irq");
    sc_core::sc_start();
}
