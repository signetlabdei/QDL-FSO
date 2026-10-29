#include "qdl-fso-atmospheric-model.h"

#include "ns3/double.h"
#include "ns3/enum.h"
#include "ns3/integer.h"
#include "ns3/log.h"
#include "ns3/qdl-fso-propagation-loss-model.h"

#include <cmath>
#include <iomanip>
#include <optional>
#ifdef DISABLE_FORTRAN_STDOUT
#include <fcntl.h>
#include <unistd.h>
#endif

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("QdlFsoAtmosphericModel");

// ------------------------------------------------------------------------- //
// Base Abstract Class: QdlFsoAtmosphericModel
// ------------------------------------------------------------------------- //

NS_OBJECT_ENSURE_REGISTERED(QdlFsoAtmosphericModel);

TypeId
QdlFsoAtmosphericModel::GetTypeId()
{
	static TypeId tid = TypeId("ns3::QdlFsoAtmosphericModel").SetParent<Object>().SetGroupName("QdlFso");
	return tid;
}

QdlFsoAtmosphericModel::QdlFsoAtmosphericModel()
{
	NS_LOG_FUNCTION(this);
}

QdlFsoAtmosphericModel::~QdlFsoAtmosphericModel()
{
	NS_LOG_FUNCTION(this);
}

void
QdlFsoAtmosphericModel::DoDispose()
{
	NS_LOG_FUNCTION(this);
	Object::DoDispose();
}

double
QdlFsoAtmosphericModel::GetAtmosphericTransmissivity(const QdlFsoPropagationContext& ctx)
{
	NS_LOG_FUNCTION(this);
	return DoGetAtmosphericTransmissivity(ctx);
}

// ------------------------------------------------------------------------- //
// Derived Class: QdlFsoKimAtmosphericModel
// ------------------------------------------------------------------------- //

NS_OBJECT_ENSURE_REGISTERED(QdlFsoKimAtmosphericModel);

TypeId
QdlFsoKimAtmosphericModel::GetTypeId()
{
	static TypeId tid =
		TypeId("ns3::QdlFsoKimAtmosphericModel")
			.SetParent<QdlFsoAtmosphericModel>()
			.SetGroupName("QdlFso")
			.AddConstructor<QdlFsoKimAtmosphericModel>()
			.AddAttribute("VisibilityKm",
						  "Meteorological visibility in kilometers (V).",
						  DoubleValue(51.0),
						  MakeDoubleAccessor(&QdlFsoKimAtmosphericModel::m_visibilityKm),
						  MakeDoubleChecker<double>())
			.AddAttribute("AdditionalAttenuationDbPerKm",
						  "Additional attenuation in dB/km.",
						  DoubleValue(0.0),
						  MakeDoubleAccessor(&QdlFsoKimAtmosphericModel::m_additionalAttenuationDbPerKm),
						  MakeDoubleChecker<double>())
			.AddAttribute("RainRateMmPerHour",
						  "Rain rate in mm/h (0 disables rain attenuation).",
						  DoubleValue(0.0),
						  MakeDoubleAccessor(&QdlFsoKimAtmosphericModel::m_rainRateMmPerHour),
						  MakeDoubleChecker<double>(0.0))
			.AddAttribute("RainCoefficientK",
						  "k in gamma = k * R^alpha (dB/km). Default: Carbonneau.",
						  DoubleValue(1.076),
						  MakeDoubleAccessor(&QdlFsoKimAtmosphericModel::m_rainCoeffK),
						  MakeDoubleChecker<double>(0.0))
			.AddAttribute("RainExponentAlpha",
						  "alpha in gamma = k * R^alpha. Default: Carbonneau.",
						  DoubleValue(0.67),
						  MakeDoubleAccessor(&QdlFsoKimAtmosphericModel::m_rainExponentAlpha),
						  MakeDoubleChecker<double>(0.0));
	return tid;
}

QdlFsoKimAtmosphericModel::QdlFsoKimAtmosphericModel()
{
}

QdlFsoKimAtmosphericModel::~QdlFsoKimAtmosphericModel()
{
}

double
QdlFsoKimAtmosphericModel::GetExponent(double visibilityKm) const
{
	NS_LOG_FUNCTION(this);

	if (visibilityKm > 50.0)
	{
		return 1.6;
	}
	else if (visibilityKm > 6.0)
	{
		return 1.3;
	}
	else if (visibilityKm > 1.0)
	{
		return 0.16 * visibilityKm + 0.34;
	}
	else if (visibilityKm > 0.5)
	{
		return visibilityKm - 0.5;
	}
	else
	{
		return 0.0;
	}
}

double
QdlFsoKimAtmosphericModel::GetAtmosphericAttenuationDbPerKm(double visibilityKm, double wavelength)
{
	NS_LOG_FUNCTION(this);

	// Prevent division by zero
	if (visibilityKm <= 0.001)
	{
		visibilityKm = 0.001;
	}

	double p = GetExponent(visibilityKm);

	return (17 / visibilityKm) * std::pow((wavelength / (550.0 * 1e-9)), -p);
}

double
QdlFsoKimAtmosphericModel::GetRainAttenuationDbPerKm() const
{
	if (m_rainRateMmPerHour <= 0.0)
	{
		return 0.0;
	}
	return m_rainCoeffK * std::pow(m_rainRateMmPerHour, m_rainExponentAlpha);
}

double
QdlFsoKimAtmosphericModel::DoGetAtmosphericTransmissivity(const QdlFsoPropagationContext& context)
{
	NS_LOG_FUNCTION(this);

	double wavelength = context.wavelength;
	double distanceKm = context.pathLength / 1e3;

	double atmAttenDbPerKm = GetAtmosphericAttenuationDbPerKm(m_visibilityKm, wavelength);
	double rainAttDbPerKm = GetRainAttenuationDbPerKm();
	double totalAttenDbPerKm = atmAttenDbPerKm + rainAttDbPerKm + m_additionalAttenuationDbPerKm;

	double totalAttenDb = totalAttenDbPerKm * distanceKm;

	// Convert dB to linear transmissivity (eta_atm)
	double eta_atm = std::pow(10.0, -totalAttenDb / 10.0);

	return eta_atm;
}

// ------------------------------------------------------------------------- //
// Derived Class: QdlFsoKruseAtmosphericModel
// ------------------------------------------------------------------------- //

NS_OBJECT_ENSURE_REGISTERED(QdlFsoKruseAtmosphericModel);

TypeId
QdlFsoKruseAtmosphericModel::GetTypeId()
{
	static TypeId tid =
		TypeId("ns3::QdlFsoKruseAtmosphericModel")
			.SetParent<QdlFsoAtmosphericModel>()
			.SetGroupName("QdlFso")
			.AddConstructor<QdlFsoKruseAtmosphericModel>()
			.AddAttribute("VisibilityKm",
						  "Meteorological visibility in kilometers (V).",
						  DoubleValue(51.0),
						  MakeDoubleAccessor(&QdlFsoKruseAtmosphericModel::m_visibilityKm),
						  MakeDoubleChecker<double>())
			.AddAttribute("AdditionalAttenuationDbPerKm",
						  "Additional attenuation in dB/km.",
						  DoubleValue(0.0),
						  MakeDoubleAccessor(&QdlFsoKruseAtmosphericModel::m_additionalAttenuationDbPerKm),
						  MakeDoubleChecker<double>())
			.AddAttribute("RainRateMmPerHour",
						  "Rain rate in mm/h (0 disables rain attenuation).",
						  DoubleValue(0.0),
						  MakeDoubleAccessor(&QdlFsoKruseAtmosphericModel::m_rainRateMmPerHour),
						  MakeDoubleChecker<double>(0.0))
			.AddAttribute("RainCoefficientK",
						  "k in gamma = k * R^alpha (dB/km). Default: Carbonneau.",
						  DoubleValue(1.076),
						  MakeDoubleAccessor(&QdlFsoKruseAtmosphericModel::m_rainCoeffK),
						  MakeDoubleChecker<double>(0.0))
			.AddAttribute("RainExponentAlpha",
						  "alpha in gamma = k * R^alpha. Default: Carbonneau.",
						  DoubleValue(0.67),
						  MakeDoubleAccessor(&QdlFsoKruseAtmosphericModel::m_rainExponentAlpha),
						  MakeDoubleChecker<double>(0.0));
	return tid;
}

QdlFsoKruseAtmosphericModel::QdlFsoKruseAtmosphericModel()
{
}

QdlFsoKruseAtmosphericModel::~QdlFsoKruseAtmosphericModel()
{
}

double
QdlFsoKruseAtmosphericModel::GetExponent(double visibilityKm) const
{
	NS_LOG_FUNCTION(this);

	if (visibilityKm > 50.0)
	{
		return 1.6;
	}
	else if (visibilityKm > 6.0)
	{
		return 1.3;
	}
	else
	{
		return 0.585 * std::cbrt(visibilityKm);
	}
}

double
QdlFsoKruseAtmosphericModel::GetAtmosphericAttenuationDbPerKm(double visibilityKm, double wavelength)
{
	NS_LOG_FUNCTION(this);

	// Prevent division by zero
	if (visibilityKm <= 0.001)
	{
		visibilityKm = 0.001;
	}

	double q = GetExponent(visibilityKm);

	return (17 / visibilityKm) * std::pow(wavelength / (550.0 * 1e-9), -q);
}

double
QdlFsoKruseAtmosphericModel::GetRainAttenuationDbPerKm() const
{
	if (m_rainRateMmPerHour <= 0.0)
	{
		return 0.0;
	}
	return m_rainCoeffK * std::pow(m_rainRateMmPerHour, m_rainExponentAlpha);
}

double
QdlFsoKruseAtmosphericModel::DoGetAtmosphericTransmissivity(const QdlFsoPropagationContext& context)
{
	NS_LOG_FUNCTION(this);

	double wavelength = context.wavelength;
	double distanceKm = context.pathLength / 1e3;

	double atmAttenDbPerKm = GetAtmosphericAttenuationDbPerKm(m_visibilityKm, wavelength);
	double rainAttDbPerKm = GetRainAttenuationDbPerKm();
	double totalAttenDbPerKm = atmAttenDbPerKm + rainAttDbPerKm + m_additionalAttenuationDbPerKm;

	double totalAttenDb = totalAttenDbPerKm * distanceKm;

	// Convert dB to linear transmissivity (eta_atm)
	double eta_atm = std::pow(10.0, -totalAttenDb / 10.0);

	return eta_atm;
}

// ------------------------------------------------------------------------- //
// Derived Class: QdlFsoLowtranAtmosphericModel
// ------------------------------------------------------------------------- //

NS_OBJECT_ENSURE_REGISTERED(QdlFsoLowtranAtmosphericModel);

TypeId
QdlFsoLowtranAtmosphericModel::GetTypeId()
{
	static TypeId tid =
		TypeId("ns3::QdlFsoLowtranAtmosphericModel")
			.SetParent<QdlFsoAtmosphericModel>()
			.SetGroupName("QdlFso")
			.AddConstructor<QdlFsoLowtranAtmosphericModel>()
			.AddAttribute("Visibility",
						  "Visibility [km]",
						  DoubleValue(0.0), // value <= 0 tells lowtran to use its own default value
						  MakeDoubleAccessor(&QdlFsoLowtranAtmosphericModel::m_visibility),
						  MakeDoubleChecker<float>())
			.AddAttribute("GroundAltitude",
						  "Ground altitude [km]",
						  DoubleValue(0.0),
						  MakeDoubleAccessor(&QdlFsoLowtranAtmosphericModel::m_groundAltitude),
						  MakeDoubleChecker<float>())
			.AddAttribute("WindSpeed",
						  "Wind speed [m/s]",
						  DoubleValue(0.0),
						  MakeDoubleAccessor(&QdlFsoLowtranAtmosphericModel::m_windSpeed),
						  MakeDoubleChecker<float>(0.0))
			.AddAttribute("AvgWindSpeed",
						  "Average wind speed [m/s]",
						  DoubleValue(0.0),
						  MakeDoubleAccessor(&QdlFsoLowtranAtmosphericModel::m_avgWindSpeed),
						  MakeDoubleChecker<float>(0.0))
			.AddAttribute("RainRate",
						  "Rain rate [mm/hr]",
						  DoubleValue(0.0),
						  MakeDoubleAccessor(&QdlFsoLowtranAtmosphericModel::m_rainRate),
						  MakeDoubleChecker<float>(0.0))
			.AddAttribute("AirMassCharacter",
						  "Air mass character (used only for Navy Maritime model) - "
						  "1 (open ocean) to 10 (strong continental influence)",
						  IntegerValue(3),
						  MakeIntegerAccessor(&QdlFsoLowtranAtmosphericModel::m_airMassCharacter),
						  MakeIntegerChecker<int>(1, 10))
			.AddAttribute("AtmosphericModel",
						  "Geographical atmospheric model",
						  EnumValue(AtmosphericModel::US_STANDARD),
						  MakeEnumAccessor<AtmosphericModel>(&QdlFsoLowtranAtmosphericModel::m_atmosphericModel),
						  MakeEnumChecker<AtmosphericModel>(AtmosphericModel::TROPICAL,
															"Tropical",
															AtmosphericModel::MIDLATITUDE_SUMMER,
															"MidlatitudeSummer",
															AtmosphericModel::MIDLATITUDE_WINTER,
															"MidlatitudeWinter",
															AtmosphericModel::SUBARCTIC_SUMMER,
															"SubarcticSummer",
															AtmosphericModel::SUBARCTIC_WINTER,
															"SubarcticWinter",
															AtmosphericModel::US_STANDARD,
															"UsStandard"))
			.AddAttribute("AerosolType",
						  "Aerosol/haze extinction model",
						  EnumValue(AerosolType::NO_AEROSOL),
						  MakeEnumAccessor<AerosolType>(&QdlFsoLowtranAtmosphericModel::m_aerosolType),
						  MakeEnumChecker<AerosolType>(AerosolType::NO_AEROSOL,
													   "NoAerosol",
													   AerosolType::RURAL_23_KM,
													   "Rural_23km",
													   AerosolType::RURAL_5_KM,
													   "Rural_5km",
													   AerosolType::NAVY,
													   "Navy",
													   AerosolType::MARITIME_23_KM,
													   "Maritime_23km",
													   AerosolType::URBAN_5_KM,
													   "Urban_5km",
													   AerosolType::TROPO_50_KM,
													   "Tropo_50km",
													   AerosolType::FOG_200_M,
													   "Fog_200m",
													   AerosolType::FOG_500_M,
													   "Fog_500m",
													   AerosolType::DESERT,
													   "Desert"))
			.AddAttribute("Season",
						  "Season: use atmospheric model default / spring - summer / fall - winter",
						  EnumValue(Season::USE_DEFAULT),
						  MakeEnumAccessor<Season>(&QdlFsoLowtranAtmosphericModel::m_season),
						  MakeEnumChecker<Season>(Season::USE_DEFAULT,
												  "UseDefault",
												  Season::SPRING_SUMMER,
												  "SpringSummer",
												  Season::FALL_WINTER,
												  "FallWinter"))
			.AddAttribute("CloudRainModel",
						  "Cloud/rain model",
						  EnumValue(CloudRainModel::NO_CLOUDS),
						  MakeEnumAccessor<CloudRainModel>(&QdlFsoLowtranAtmosphericModel::m_couldRainModel),
						  MakeEnumChecker<CloudRainModel>(CloudRainModel::NO_CLOUDS,
														  "NoClouds",
														  CloudRainModel::CUMULUS_660_M_2_7_KM,
														  "Cumulus_660m_2.7km",
														  CloudRainModel::ALTOSTRATUS_2_4_KM_3_KM,
														  "Altostratus_2.4km_3km",
														  CloudRainModel::STRATUS_330_M_1_KM,
														  "Stratus_330m_1km",
														  CloudRainModel::STRATUS_660_M_1_KM,
														  "Stratus_660m_1km",
														  CloudRainModel::NIMBOSTRATUS_160_M_660_M,
														  "Nimbostratus_160m_660m",
														  CloudRainModel::DRIZZLE_2MM_HR,
														  "Drizzle_2mm_hr",
														  CloudRainModel::LIGHT_RAIN_5MM_HR,
														  "LightRain_5mm_hr",
														  CloudRainModel::MODERATE_RAIN_12_5MM_HR,
														  "ModerateRain_12.5mm_hr",
														  CloudRainModel::HEAVY_RAIN_25MM_HR,
														  "HeavyRain_25mm_hr",
														  CloudRainModel::EXTREME_RAIN_75MM_HR,
														  "ExtremeRain_75mm_hr"))
			.AddAttribute("VolcanicTypeProfile",
						  "Volcanic aerosol profile/extinction type",
						  EnumValue(VolcanicTypeProfile::DEFAULT),
						  MakeEnumAccessor<VolcanicTypeProfile>(&QdlFsoLowtranAtmosphericModel::m_volcanicTypeProfile),
						  MakeEnumChecker<VolcanicTypeProfile>(VolcanicTypeProfile::DEFAULT,
															   "Default (StratosphericBackground)",
															   VolcanicTypeProfile::STRATOSPHERIC_BACKGROUND,
															   "StratosphericBackground",
															   VolcanicTypeProfile::AGED_VOLC_MODERATE_VOLC,
															   "AgedVolcModerateVolc",
															   VolcanicTypeProfile::FRESH_VOLC_HIGH_VOLC,
															   "FreshVolcHighVolc",
															   VolcanicTypeProfile::AGED_VOLC_HIGH_VOLC,
															   "AgedVolcHighVolc",
															   VolcanicTypeProfile::FRESH_VOLC_MODERATE_VOLC,
															   "FreshVolcModerateVolc",
															   VolcanicTypeProfile::BACKGROUND_STRATO_MODERATE_VOLC,
															   "BackgroundStratoModerateVolc",
															   VolcanicTypeProfile::BACKGROUND_STRATO_HIGH_VOLC,
															   "BackgroundStratoHighVolc",
															   VolcanicTypeProfile::FRESH_VOLC_EXTREME_VOLC,
															   "FreshVolcExtremeVolc"));
	return tid;
}

QdlFsoLowtranAtmosphericModel::QdlFsoLowtranAtmosphericModel()
{
}

QdlFsoLowtranAtmosphericModel::~QdlFsoLowtranAtmosphericModel()
{
}

// ---- C-compatible interface implemented by the Fortran bind(C) wrapper ----
extern "C"
{
	void lowtran_transmittance(float wavelength_nm,
							   int model,		// atmosphere model
							   int itype,		// path type (horizontal / slant atmospheric / slant to space)
							   float h1,		// transmitter altitude [km]
							   float h2,		// receiver altitude [km]
							   float angle,		// zenit angle [deg]
							   float range_km,	// slant path length [km]
							   int ihaze,		// aerosol/haze model (0-10)
							   float vis_km,	// surface visibility [km], <=0 => IHAZE default
							   int iseasn,		// season (0=default,1=spring/summer,2=fall/winter)
							   int icstl,		// air-mass character (1-10), maritime IHAZE only
							   int icld,		// cloud/rain model (0-19; avoid 18-20)
							   int ivulcn,		// volcanic aerosol model (0-7)
							   float wss,		// wind speed [m/s] needed for ihaze=3 and ihaze=10 (navy and desert)
							   float whh,		// 24h average wind speed [m/s] for ihaze=3 (navy)
							   float rain_rate, // rain rate [mm/hr]
							   float gndalt_km, // ground altitude [km]
							   float* transmittance,
							   float* wavelength_out_nm);
}

// This function has been made part of QdlFsoLowtranAtmosphericModel to access the definition of PathType
std::optional<float>
QdlFsoLowtranAtmosphericModel::lowtran_cpp_wrapper(
	float wavelength_nm,
	AtmosphericModel model,
	PathType itype,
	float h1,
	float h2,
	float angle,				// zenit angle [deg]
	float range_km,				// slant path length [km]
	AerosolType ihaze,			// aerosol/haze model (0-10)
	float vis_km,				// surface visibility [km], <=0 => IHAZE default
	Season iseasn,				// season (0=default,1=spring/summer,2=fall/winter)
	int icstl,					// air-mass character (1-10), maritime IHAZE only
	CloudRainModel icld,		// cloud/rain model (0-19; avoid 18-20)
	VolcanicTypeProfile ivulcn, // volcanic aerosol model (0-7)
	float wss,					// wind speed [m/s] needed for ihaze=3 and ihaze=10 (navy and desert)
	float whh,					// 24h average wind speed [m/s] for ihaze=3 (navy)
	float rain_rate,			// rain rate [mm/hr]
	float gndalt_km				// ground altitude [km]
)
{
#ifdef DISABLE_FORTRAN_STDOUT
	/*
	// From Claude
	#include <unistd.h>
	#include <fcntl.h>

	void call_fortran_silently() {
		fflush(stdout);
		int saved_stdout = dup(1);                  // save original fd 1
		int devnull = open("/dev/null", O_WRONLY);
		dup2(devnull, 1);                           // fd 1 now points to /dev/null
		close(devnull);

		fortran_function_();                        // call your Fortran routine

		fflush(stdout);
		dup2(saved_stdout, 1);                      // restore original fd 1
		close(saved_stdout);
	}
	*/
	fflush(stdout);
	int saved_stdout = dup(STDOUT_FILENO);
	int devnull = open("/dev/null", O_WRONLY);
	dup2(devnull, STDOUT_FILENO);
	close(devnull);
#endif
	float out_transmittance = -1.0;
	float out_wavelength_nm = 0.0; // ignored

	lowtran_transmittance(wavelength_nm,
						  static_cast<int>(model),
						  static_cast<int>(itype),
						  h1,
						  h2,
						  angle,
						  range_km,
						  static_cast<int>(ihaze),
						  vis_km,
						  static_cast<int>(iseasn),
						  icstl,
						  static_cast<int>(icld),
						  static_cast<int>(ivulcn),
						  wss,
						  whh,
						  rain_rate,
						  gndalt_km,
						  &out_transmittance,
						  &out_wavelength_nm);
#ifdef DISABLE_FORTRAN_STDOUT
	fflush(stdout);
	dup2(saved_stdout, STDOUT_FILENO);
	close(saved_stdout);
#endif
	if (std::abs(out_wavelength_nm - wavelength_nm) > 1) // 1 nm tolerance chosen arbitrarily for this sanity check
	{
		// If out_wavelength_nm == 0.0 it is usually because Lowtran crashed in some way, or because it did not like its
		// input parameters
		NS_LOG_ERROR("QdlFsoLowtranAtmosphericModel::GetAtmosphericTransmissivity mismatched wavelengths in " +
					 std::to_string(wavelength_nm) + " out " + std::to_string(out_wavelength_nm));
		return std::nullopt;
	}

	if (out_transmittance < 0 || out_transmittance > 1)
	{
		NS_LOG_ERROR("QdlFsoLowtranAtmosphericModel::GetAtmosphericTransmissivity invalid transmittance result " +
					 std::to_string(out_transmittance));
		return std::nullopt;
	}

	return out_transmittance;
}

static double
rad_to_deg(double rad)
{
	return rad / M_PI * 180.0;
}

double
QdlFsoLowtranAtmosphericModel::DoGetAtmosphericTransmissivity(const QdlFsoPropagationContext& context)
{
	NS_LOG_FUNCTION(this);

	const float wavelength_nm = context.wavelength * 1e9;
	const float up_h_km = std::max(context.txAltitude, context.rxAltitude) / 1000.0;
	const float down_h_km = std::min(context.txAltitude, context.rxAltitude) / 1000.0;
	PathType itype = (up_h_km == down_h_km) ? PathType::HORIZONTAL
					 : (up_h_km >= 100000)	? PathType::SLANT_TO_SPACE
											: PathType::SLANT_ATMOSPHERIC;

	float angle_deg = rad_to_deg(context.zenithAngle); // zenith angle [deg]

	const float range_km = context.pathLength / 1000.0;
	if (angle_deg > 90.0)
	{
		angle_deg = 180.0 - angle_deg;
	}
	if (angle_deg == 0.0)
	{
		angle_deg = 0.1; // lowtran_transmittance call seems to sometimes hang for non-satellite slant paths when the
						 // zenit angle is exactly = 0 deg
	}
	NS_ASSERT_MSG(angle_deg >= 0.0 && angle_deg <= 90.0,
				  "invalid deg zenit angle " + std::to_string(angle_deg) + " ( rad " +
					  std::to_string(context.zenithAngle) + ")");

	std::optional<float> opt_trasmittance =
		lowtran_cpp_wrapper(wavelength_nm,				 // wavelength [nm]
							this->m_atmosphericModel,	 // atmosphere model
							itype,						 // path type
							down_h_km,					 // tx altitude [km]
							up_h_km,					 // rx altitude [km]
							angle_deg,					 // zenit angle [deg]
							range_km,					 // range [km]
							this->m_aerosolType,		 // aerosol model
							this->m_visibility,			 // surface visibility [km]
							this->m_season,				 // season
							this->m_airMassCharacter,	 // air-mass character (maritime IHAZE only)
							this->m_couldRainModel,		 // cloud/rain model
							this->m_volcanicTypeProfile, // volcanic aerosol model
							this->m_windSpeed,			 // current wind speed
							this->m_avgWindSpeed,		 // average wind speed
							this->m_rainRate,			 // rain rate
							this->m_groundAltitude		 // ground altitude [km]
		);
	if (opt_trasmittance.has_value())
	{
		return static_cast<double>(*opt_trasmittance);
	}
	NS_LOG_ERROR("QdlFsoLowtranAtmosphericModel::GetAtmosphericTransmissivity call to lowtran_cpp_wrapper failed");

	return 0.0;
}

} // namespace ns3
