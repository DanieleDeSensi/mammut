#include "mammut.hpp"
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;

PYBIND11_MODULE(mammut, m) {
    py::class_<mammut::energy::Counter, std::unique_ptr<mammut::energy::Counter, py::nodelete>>(m, "Counter")
        .def("getJoules", &mammut::energy::Counter::getJoules)
        .def("reset", &mammut::energy::Counter::reset)
        .def("getType", &mammut::energy::Counter::getType);

    py::class_<mammut::energy::CounterCpus, mammut::energy::Counter, std::unique_ptr<mammut::energy::CounterCpus, py::nodelete>>(m, "CounterCpus")
        .def("getJoules", &mammut::energy::CounterCpus::getJoules)
        .def("reset", &mammut::energy::CounterCpus::reset)
        .def("getType", &mammut::energy::CounterCpus::getType);

    py::class_<mammut::energy::Energy, std::unique_ptr<mammut::energy::Energy, py::nodelete>>(m, "Energy")
        .def("getCounter", (mammut::energy::Counter*(mammut::energy::Energy::*)(void) const)                        &mammut::energy::Energy::getCounter)
        .def("getCounter", (mammut::energy::Counter*(mammut::energy::Energy::*)(mammut::energy::CounterType) const) &mammut::energy::Energy::getCounter)
        .def("getCountersTypes", &mammut::energy::Energy::getCountersTypes);

    py::enum_<mammut::energy::CounterType>(m, "CounterType", py::arithmetic())
            .value("COUNTER_CPUS", mammut::energy::CounterType::COUNTER_CPUS)
            .value("COUNTER_PLUG", mammut::energy::CounterType::COUNTER_PLUG)
            .export_values();

    py::class_<mammut::Mammut>(m, "Mammut")
        .def(py::init<>())
        .def(py::init<const mammut::Mammut&>())
        .def("getInstanceEnergy", &mammut::Mammut::getInstanceEnergy);
}
