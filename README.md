# QDL-FSO - A Free-Space Optical Channel Model for Classical and Quantum Network Research in ns-3

[![ns-3](https://img.shields.io/badge/ns--3-3.47-brightgreen?style=flat-square)](https://www.nsnam.org/)
[![Q2NS](https://img.shields.io/badge/compatible%20with-Q2NS-blue?style=flat-square)](https://github.com/QuantumInternet-it/q2ns)

**QDL-FSO** is a modular and extensible Free-Space Optical (FSO) channel model for the [ns-3](https://www.nsnam.org/) network simulator. It computes the stochastic transmissivity of an FSO link from first-principles contributions (hardware efficiency, atmospheric attenuation, beam spreading and wandering, pointing error, and scintillation) and exposes it through two interfaces: as a standard ns-3 propagation loss model for **classical** optical links, and, through integration with [Q2NS](https://github.com/QuantumInternet-it/q2ns), as a **quantum** channel that applies loss and dephasing to photonic qubits.

## Overview

FSO communication is a promising technology for next-generation wireless and non-terrestrial networks, thanks to its high data rates, large bandwidth, directivity, and resistance to interception. For quantum networks, FSO links additionally enable the wireless transmission of photons encoding quantum information beyond the distance and flexibility limits of optical fibers, with applications ranging from terrestrial links to aerial and satellite platforms. In both cases, performance is dictated by a dynamic propagation environment: atmospheric attenuation, turbulence, and pointing errors introduce significant time-varying loss and, for quantum states, noise.

Existing ns-3 FSO tools either support only satellite downlinks with simplified propagation models that neglect beam wandering and pointing errors, or provide no quantum channel modeling at all. QDL-FSO fills this gap with a single, physically grounded channel model that is native to ns-3 and shared by classical and quantum studies, so that both are driven by the same link conditions.

The link transmissivity is modeled as the product of independent contributions,

```
η_ch = η_tx · η_rx · η_atm · η_gpe · η_scint
```

where each factor is implemented as a separate, replaceable ns-3 component:

| Factor | Effect | Model |
| --- | --- | --- |
| `η_tx`, `η_rx` | Transmitter / receiver hardware efficiency | `QdlFsoOpticalTerminal` |
| `η_atm` | Atmospheric attenuation | Kim, Kruse, or LOWTRAN |
| `η_gpe` | Beam spreading and wandering, pointing error | `QdlFsoGeometricPointingErrorModel` |
| `η_scint` | Scintillation | Gamma-Gamma (`QdlFsoScintillationModel`) |

Main features:

- **Link geometries:** horizontal, uplink, and downlink vertical links, with terrestrial, aerial, or space terminals.
- **Atmospheric conditions:** visibility-based (Kim, Kruse) and numerical ([LOWTRAN](https://apps.dtic.mil/sti/citations/ADA206773)) attenuation, including rain and Hufnagel-Valley turbulence profiles.
- **Blockage:** line-of-sight evaluation against ns-3 `buildings`; the transmissivity is set to zero in NLoS conditions.
- **Classical links:** `QdlFsoPropagationLossModel` extends the ns-3 `PropagationLossModel` and computes the received optical power as `P_rx = P_tx · η_ch`, so it can be queried like any other ns-3 propagation loss model.
- **Quantum links:** `QdlFsoQChannel` extends the Q2NS `QChannel`. Qubit loss is modeled as an erasure channel with probability `1 − η_ch`, and turbulence-induced dephasing as a function of the Rytov variance. The channel is updated at every transmission to follow time-varying conditions.
- **Validation:** scripts and figures comparing the model against the theoretical results in the literature are provided in [`validation/`](validation).

> [!NOTE]
> Classical support is currently at the propagation level (received optical power). Physical- and link-layer models of classical FSO links are yet to be addressed.

## Repository Structure

| Folder | Content |
| --- | --- |
| `model/` | Channel, atmospheric, turbulence, pointing, scintillation, and terminal models |
| `helper/` | `QdlFsoHelper`, for the creation and configuration of FSO links |
| `lowtran/` | LOWTRAN source code used for numerical atmospheric attenuation |
| `examples/` | Example simulations, including the quantum entanglement-distribution scenario of the paper |
| `validation/` | Validation scripts and plots |

## Getting Started

### Dependencies

QDL-FSO is an ns-3 `contrib` module. Its quantum functionalities rely on Q2NS, which requires a few small hooks (e.g., exposing `QChannel::SendFrom()` to derived classes and adding qubit-reception traces) that are not available in the upstream release.

> [!IMPORTANT]
> **Do not use the upstream Q2NS repository.** Use our modified fork, branch `QDL-FSO-support`: [https://github.com/signetlabdei/q2ns](https://github.com/signetlabdei/q2ns). The changes are limited to the hooks needed by QDL-FSO.

> [!NOTE]
> QDL-FSO is developed and tested with **ns-3.47**. This is also the version targeted by Q2NS, so the same ns-3 release must be used for both modules. Other versions may require adjustments.

Further requirements follow those of ns-3 and Q2NS:
- C++23 compiler (GCC >= 11 / Clang >= 17)
- CMake >= 3.20
- Fortran compiler to build the bundled LOWTRAN code.

### 1. Clone ns-3 and the modules

```bash
git clone https://gitlab.com/nsnam/ns-3-dev.git
cd ns-3-dev
git checkout ns-3.47

# Q2NS: use the QDL-FSO-compatible fork
git clone -b QDL-FSO-support https://github.com/signetlabdei/q2ns.git contrib/q2ns

# QDL-FSO
git clone https://github.com/signetlabdei/QDL-FSO.git contrib/qdlfso
```

### 2. Configure and build

```bash
./ns3 configure --enable-examples --enable-tests
./ns3 build
```

### 3. Run the quantum example

```bash
./ns3 run qdl-fso-quantum-example
```

This reproduces the scenario of the paper (Sec. IV-B): a quantum base station distributes entangled qubits to a mobile user over a horizontal FSO link under varying atmospheric conditions, reporting the transmissivity, qubit loss, and state fidelity over time.

## Usage

Both use cases share the same optical terminals (`QdlFsoOpticalTerminal`: wavelength, transmitter beam waist, receiver aperture, hardware and tracking efficiency, pointing jitter) and the same channel model (`QdlFsoPropagationLossModel`). The latter can be pre-configured by the user or left to the default models provided by `QdlFsoHelper`.

### Classical links: received optical power

`QdlFsoPropagationLossModel` implements `DoCalcRxPower()`, which ns-3 invokes through the public `CalcRxPower()` method of `PropagationLossModel`. The received power is `P_rx = P_tx · η_ch`, i.e., in logarithmic units `P_rx [dBm] = P_tx [dBm] + 10 log10(η_ch)`. Since the scintillation, beam wandering, and pointing error terms are random variables, **each call returns a new channel realization**: the mean transmissivity is estimated by averaging over multiple calls.

```cpp
// You can find this code in: examples/qdl-fso-classical-simple-example.cc
#include "ns3/core-module.h"
#include "ns3/mobility-module.h"
#include "ns3/qdl-fso-propagation-loss-model.h"
#include "ns3/qdl-fso-optical-terminal.h"
#include "ns3/qdl-fso-helper.h"
#include "ns3/buildings-module.h"

using namespace ns3;

int main() {
// Nodes: a 20 m high transmitter and a receiver 1 km away at 1.5 m
Ptr<Node> tx_node = CreateObject<Node>();
Ptr<Node> rx_node = CreateObject<Node>();
Ptr<ConstantPositionMobilityModel> tx_mob = CreateObject<ConstantPositionMobilityModel>();
Ptr<ConstantPositionMobilityModel> rx_mob = CreateObject<ConstantPositionMobilityModel>();
tx_mob->SetPosition(Vector(0.0, 0.0, 20.0));
rx_mob->SetPosition(Vector(1000.0, 0.0, 1.5));
tx_node->AggregateObject(tx_mob);
rx_node->AggregateObject(rx_mob);
// Add bulding info to the mobility model (this is purely a "coding" necessity, it is not something logical)
BuildingsHelper::Install(tx_node);
BuildingsHelper::Install(rx_node);

// Terminals
Ptr<QdlFsoOpticalTerminal> tx_term = CreateObject<QdlFsoOpticalTerminal>();
tx_term->SetWavelength(1550e-9);		// 1550 nm
tx_term->SetTxBeamWaist(0.10);		// 10 cm radius
tx_term->SetPointingJitter(1.0e-6);	// 1 μrad
tx_term->SetRxApertureDiameter(0.3);	// 30 cm diameter
tx_term->SetTrackingEfficiency(0.85); // 100% tracking efficiency
tx_term->SetHardwareEfficiency(1.0);	// 100% hardware efficiency
tx_term->SetTxPower(30.0); // 1 W

Ptr<QdlFsoOpticalTerminal> rx_term = CreateObject<QdlFsoOpticalTerminal>();
rx_term->SetWavelength(1550e-9);	  // 1550 nm
rx_term->SetTxBeamWaist(0.10);		  // 10 cm radius
rx_term->SetPointingJitter(1.0e-6);	  // 1 μrad
rx_term->SetRxApertureDiameter(0.3);  // 30 cm diameter
rx_term->SetTrackingEfficiency(0.85); // 100% tracking efficiency
rx_term->SetHardwareEfficiency(1.0);  // 100% hardware efficiency

// FSO channel: configure atmospheric, turbulence, pointing, and scintillation models
// (or rely on the defaults) and attach the optical terminals of the two endpoints
ns3::Ptr<QdlFsoHelper> helper = CreateObject<QdlFsoHelper>();
Ptr<QdlFsoPropagationLossModel> loss = helper->CreateDefaultPropagationLossModel();
loss->SetAttribute("IsQuantum", BooleanValue(false));
loss->SetTerminals(tx_term, tx_mob, rx_term, rx_mob);


// Received power over 1000 realizations
const uint32_t nRuns = 1000;
double sumPower = 0.0;
double sumEta = 0.0;
for (uint32_t i = 0; i < nRuns; ++i)
{
    double rxPowerDbm = loss->CalcRxPower(tx_term->GetTxPower(), tx_mob, rx_mob);
	sumPower += rxPowerDbm;
    sumEta += std::pow(10.0, (rxPowerDbm - tx_term->GetTxPower()) / 10.0); // eta_ch of this realization
}
std::cout << "Mean rx power: " << sumPower / nRuns << std::endl;
std::cout << "Mean transmissivity: " << sumEta / nRuns << std::endl;
}
```

The same model can be chained with other ns-3 `PropagationLossModel` instances via `SetNext()`, and, if the link is obstructed by a building (ns-3 `buildings` module), `η_ch = 0` and the received power vanishes.

### Quantum links: qubit loss and dephasing

Quantum links are created through `QdlFsoHelper::InstallQdlFsoQuantumLink()`, following the approach of `q2ns::NetController`. The channel can be set up either with a user-defined `QdlFsoPropagationLossModel` or with the default models provided by the helper. At each transmission, `QdlFsoQChannel` converts `η_ch` into a survival probability (`GetSurvivalProbability()`) and updates the loss and dephasing maps applied to the qubit.

```cpp
// You can find this code in: examples/qdl-fso-quantum-simple-example.cc
#include "ns3/buildings-module.h"
#include "ns3/core-module.h"
#include "ns3/mobility-module.h"
#include "ns3/q2ns-analysis.h"
#include "ns3/q2ns-netcontroller.h"
#include "ns3/q2ns-qnode.h"
#include "ns3/q2ns-qstate.h"
#include "ns3/q2ns-qubit.h"
#include "ns3/qdl-fso-helper.h"
#include "ns3/qdl-fso-optical-terminal.h"
#include "ns3/qdl-fso-propagation-loss-model.h"

#include <iostream>

using namespace ns3;
using namespace q2ns;

namespace
{
uint32_t nSent = 0;
uint32_t nReceived = 0;
double sumEta = 0.0;
double sumFidelity = 0.0;

// QdlFsoQChannel TxTrace: (qubit, transmissivity, dephasing prob, src node, dst node)
void
OnTx(std::shared_ptr<Qubit> q, double eta, double dephProb, Ptr<const Node> src, Ptr<const Node> dst)
{
    sumEta += eta;
}
} // namespace

int
main()
{
    RngSeedManager::SetSeed(42);
    RngSeedManager::SetRun(1);

    // Q2NS controller and quantum nodes
    NetController net;
    net.SetQStateBackend(QStateBackend::Ket);
    auto tx_node = net.CreateNode("Alice");
    auto rx_node = net.CreateNode("Bob");

    // Mobility: a 20 m high transmitter and a receiver 1 km away at 1.5 m
    MobilityHelper mobility;
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(tx_node);
    mobility.Install(rx_node);
    Ptr<MobilityModel> tx_mob = tx_node->GetObject<MobilityModel>();
    Ptr<MobilityModel> rx_mob = rx_node->GetObject<MobilityModel>();
    tx_mob->SetPosition(Vector(0.0, 0.0, 20.0));
    rx_mob->SetPosition(Vector(1000.0, 0.0, 1.5));
    // Add building info to the mobility model (this is purely a "coding" necessity, it is not something logical)
    BuildingsHelper::Install(tx_node);
    BuildingsHelper::Install(rx_node);

    // Terminals
    Ptr<QdlFsoOpticalTerminal> tx_term = CreateObject<QdlFsoOpticalTerminal>();
    tx_term->SetWavelength(1550e-9);      // 1550 nm
    tx_term->SetTxBeamWaist(0.10);        // 10 cm radius
    tx_term->SetPointingJitter(1.0e-6);   // 1 μrad
    tx_term->SetRxApertureDiameter(0.3);  // 30 cm diameter
    tx_term->SetTrackingEfficiency(0.85); // 85% tracking efficiency
    tx_term->SetHardwareEfficiency(1.0);  // 100% hardware efficiency

    Ptr<QdlFsoOpticalTerminal> rx_term = CreateObject<QdlFsoOpticalTerminal>();
    rx_term->SetWavelength(1550e-9);
    rx_term->SetTxBeamWaist(0.10);
    rx_term->SetPointingJitter(1.0e-6);
    rx_term->SetRxApertureDiameter(0.3);
    rx_term->SetTrackingEfficiency(0.85);
    rx_term->SetHardwareEfficiency(1.0);

    // FSO quantum channel: default propagation loss model is created and installed through the helper
    Ptr<QdlFsoHelper> helper = CreateObject<QdlFsoHelper>();
    auto ch = helper->InstallQdlFsoQuantumLink(tx_node, rx_node, tx_term, rx_term);

    // Apply turbulence-induced dephasing to the flying qubits, in addition to loss
    ch->SetAttribute("EnableDephasing", BooleanValue(true));
    ch->TraceConnectWithoutContext("QubitTxedTrace", MakeCallback(&OnTx));

    // Reference Bell pair, used to compute the fidelity of the received state
    auto refNode = net.CreateNode("ReferenceNode");
    auto [qRefA, qRefB] = refNode->CreateBellPair();
    auto sRef = net.GetState(qRefA);

    // Bob's receive callback: compute the fidelity of the received state
    rx_node->SetRecvCallback([&net, sRef](std::shared_ptr<Qubit> q) {
        auto st = net.GetState(q);
        if (st)
        {
            sumFidelity += q2ns::analysis::Fidelity(*sRef, *st);
        }
        ++nReceived;
    });

    // Alice generates and sends one Bell pair every 10 ms (100 pairs/s), keeping one qubit
    const uint32_t nRuns = 1000;
    for (uint32_t i = 0; i < nRuns; ++i)
    {
        Simulator::Schedule(MilliSeconds(10 * i), [tx_node, rx_node]() {
            auto [qA, qB] = tx_node->CreateBellPair();
            tx_node->Send(qB, rx_node->GetId());
            ++nSent;
        });
    }

    Simulator::Stop(Seconds(11.0));
    Simulator::Run();
    Simulator::Destroy();

    std::cout << "Mean transmissivity:  " << sumEta / nSent << std::endl;
    std::cout << "Qubit loss:           " << 100.0 * (nSent - nReceived) / nSent << " %" << std::endl;
    std::cout << "Mean fidelity (RX):   " << (nReceived ? sumFidelity / nReceived : 0.0) << std::endl;
    return 0;
};
```

## Paper

This repository accompanies the following paper, which has been submitted to the *IEEE International Conference on Communications (ICC) 2027*:

> B. Morellato, R. Di Bella, M. Zorzi, and M. Giordani, "Implementation of a Free-Space Optical Channel Model in ns-3 For Quantum Network Research," submitted to *IEEE ICC 2027*.

If you use QDL-FSO in your research, please cite:
```bibtex
@inproceedings{morellato2027qdlfso,
  title     = {Implementation of a {Free-Space} Optical Channel Model in ns-3 For Quantum Network Research},
  author    = {Morellato, Bartolomeo and Di Bella, Riccardo and Zorzi, Michele and Giordani, Marco},
  booktitle = {ICC 2027 - IEEE International Conference on Communications: SAC - Quantum
  Communications and Information Technology (IEEE ICC 2027 - SACqcc)},
  year      = {2027},
  note      = {Under review}
}
```

If you use it for quantum networking research, please also cite Q2NS, on which the quantum simulation relies:

> A. Pearson, F. Mazza, M. Caleffi, and A. S. Cacciapuoti, "An extensible quantum network simulator built on ns-3: Q2NS design and evaluation," *Computer Networks*, vol. 283, p. 112292, Jun. 2026. [doi:10.1016/j.comnet.2026.112292](https://doi.org/10.1016/j.comnet.2026.112292)

## Future Work

Planned extensions include temporal correlation of transmissivity samples, physical- and link-layer models of classical FSO links, quantum memory decoherence, a stochastic cloud-blockage channel condition model, and receiver background noise and detector dark counts.

## Authors

Department of Information Engineering, University of Padova, Italy ([SIGNET](https://github.com/signetlabdei)):
Bartolomeo Morellato, Riccardo Di Bella, Michele Zorzi, Marco Giordani.

## License

TBD