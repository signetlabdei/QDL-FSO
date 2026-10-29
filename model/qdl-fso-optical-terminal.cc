#include "qdl-fso-optical-terminal.h"

#include "ns3/double.h"
#include "ns3/log.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("QdlFsoOpticalTerminal");

NS_OBJECT_ENSURE_REGISTERED(QdlFsoOpticalTerminal);

TypeId
QdlFsoOpticalTerminal::GetTypeId()
{
	static TypeId tid = TypeId("ns3::QdlFsoOpticalTerminal")
							.SetParent<Object>()
							.SetGroupName("QdlFso")
							.AddConstructor<QdlFsoOpticalTerminal>()
							.AddAttribute("Wavelength",
										  "Operating optical wavelength in meters.",
										  DoubleValue(1550e-9), // Default: 1550 nm
										  MakeDoubleAccessor(&QdlFsoOpticalTerminal::m_wavelength),
										  MakeDoubleChecker<double>())
							.AddAttribute("RxApertureDiameter",
										  "Receiver aperture diameter in meters.",
										  DoubleValue(0.1),
										  MakeDoubleAccessor(&QdlFsoOpticalTerminal::m_rxApertureDiameter),
										  MakeDoubleChecker<double>())
							.AddAttribute("TxBeamWaist",
										  "Transmitter beam waist (radius) in meters.",
										  DoubleValue(0.025),
										  MakeDoubleAccessor(&QdlFsoOpticalTerminal::m_txBeamWaist),
										  MakeDoubleChecker<double>())
							.AddAttribute("HardwareEfficiency",
										  "Internal optical hardware efficiency [0.0 - 1.0].",
										  DoubleValue(1.0),
										  MakeDoubleAccessor(&QdlFsoOpticalTerminal::m_hardwareEfficiency),
										  MakeDoubleChecker<double>(0.0, 1.0))
							.AddAttribute("TrackingEfficiency",
										  "Acquisition and tracking efficiency [0.0 - 1.0].",
										  DoubleValue(1.0),
										  MakeDoubleAccessor(&QdlFsoOpticalTerminal::m_trackingEfficiency),
										  MakeDoubleChecker<double>(0.0, 1.0))
							.AddAttribute("PointingJitter",
										  "RMS pointing jitter angle in radians.",
										  DoubleValue(1e-6),
										  MakeDoubleAccessor(&QdlFsoOpticalTerminal::m_pointingJitter),
										  MakeDoubleChecker<double>())
							.AddAttribute("TxPower",
										  "Transmitter optical power in Watts (Classical).",
										  DoubleValue(0.01),
										  MakeDoubleAccessor(&QdlFsoOpticalTerminal::m_txPower),
										  MakeDoubleChecker<double>());
	//   .AddAttribute("TxPulseRate",
	//                  "Transmitter pulse repetition rate in Hz (Quantum).",
	//                  DoubleValue(1e8),
	//                  MakeDoubleAccessor(&QdlFsoOpticalTerminal::m_txPulseRate),
	//                  MakeDoubleChecker<double>())
	//   .AddAttribute("TxMeanPhotonNumber",
	//                  "Mean photon number per pulse (Quantum).",
	//                  DoubleValue(0.1),
	//                  MakeDoubleAccessor(&QdlFsoOpticalTerminal::m_txMeanPhotonNumber),
	//                  MakeDoubleChecker<double>());

	return tid;
}

QdlFsoOpticalTerminal::QdlFsoOpticalTerminal()
	: m_wavelength(1550e-9),
	  m_rxApertureDiameter(0.1),
	  m_txBeamWaist(0.25),
	  m_hardwareEfficiency(1.0),
	  m_trackingEfficiency(1.0),
	  m_pointingJitter(1e-6),
	  m_txPower(0.01)
//   m_txPulseRate(1e8),
//   m_txMeanPhotonNumber(0.1)
{
	NS_LOG_FUNCTION(this);
}

QdlFsoOpticalTerminal::~QdlFsoOpticalTerminal()
{
	NS_LOG_FUNCTION(this);
}

void
QdlFsoOpticalTerminal::DoDispose()
{
	NS_LOG_FUNCTION(this);
	Object::DoDispose();
}

double
QdlFsoOpticalTerminal::GetWavelength() const
{
	NS_LOG_FUNCTION(this);
	return m_wavelength;
}

double
QdlFsoOpticalTerminal::GetRxApertureDiameter() const
{
	NS_LOG_FUNCTION(this);
	return m_rxApertureDiameter;
}

double
QdlFsoOpticalTerminal::GetTxBeamWaist() const
{
	NS_LOG_FUNCTION(this);
	return m_txBeamWaist;
}

double
QdlFsoOpticalTerminal::GetHardwareEfficiency() const
{
	NS_LOG_FUNCTION(this);
	return m_hardwareEfficiency;
}

double
QdlFsoOpticalTerminal::GetTrackingEfficiency() const
{
	NS_LOG_FUNCTION(this);
	return m_trackingEfficiency;
}

double
QdlFsoOpticalTerminal::GetPointingJitter() const
{
	NS_LOG_FUNCTION(this);
	return m_pointingJitter;
}

double
QdlFsoOpticalTerminal::GetTxPower() const
{
	NS_LOG_FUNCTION(this);
	return m_txPower;
}

void
QdlFsoOpticalTerminal::SetWavelength(double wavelength)
{
	NS_LOG_FUNCTION(this);
	m_wavelength = wavelength;
}

void
QdlFsoOpticalTerminal::SetRxApertureDiameter(double rxApertureDiameter)
{
	NS_LOG_FUNCTION(this);
	m_rxApertureDiameter = rxApertureDiameter;
}

void
QdlFsoOpticalTerminal::SetTxBeamWaist(double txBeamWaist)
{
	NS_LOG_FUNCTION(this);
	m_txBeamWaist = txBeamWaist;
}

void
QdlFsoOpticalTerminal::SetHardwareEfficiency(double hardwareEfficiency)
{
	NS_LOG_FUNCTION(this);
	m_hardwareEfficiency = hardwareEfficiency;
}

void
QdlFsoOpticalTerminal::SetTrackingEfficiency(double trackingEfficiency)
{
	NS_LOG_FUNCTION(this);
	m_trackingEfficiency = trackingEfficiency;
}

void
QdlFsoOpticalTerminal::SetPointingJitter(double pointingJitter)
{
	NS_LOG_FUNCTION(this);
	m_pointingJitter = pointingJitter;
}

void
QdlFsoOpticalTerminal::SetTxPower(double txPower)
{
	NS_LOG_FUNCTION(this);
	m_txPower = txPower;
}

// double QdlFsoOpticalTerminal::GetTxPulseRate() const {
// 	 NS_LOG_FUNCTION(this);
//   return m_txPulseRate;
// }

// double QdlFsoOpticalTerminal::GetTxMeanPhotonNumber() const {
//	 NS_LOG_FUNCTION(this);
//   return m_txMeanPhotonNumber;
// }

// void QdlFsoOpticalTerminal::SetTxPulseRate(double txPulseRate) {
//	 NS_LOG_FUNCTION(this);
//   m_txPulseRate = txPulseRate;
// }

// void QdlFsoOpticalTerminal::SetTxMeanPhotonNumber(double txMeanPhotonNumber) {
//	 NS_LOG_FUNCTION(this);
//   m_txMeanPhotonNumber = txMeanPhotonNumber;
// }

} // namespace ns3
