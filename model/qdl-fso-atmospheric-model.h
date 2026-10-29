#ifndef QDL_FSO_ATMOSPHERIC_MODEL_H
#define QDL_FSO_ATMOSPHERIC_MODEL_H

#include "ns3/object.h"
#include "ns3/type-id.h"

#include <optional>

// #define DISABLE_FORTRAN_STDOUT

namespace ns3
{

struct QdlFsoPropagationContext;

/**
 * @ingroup qdl-fso
 * @brief  The abstract base class of an Atmospheric Model for the qdl-fso module [Morellato2027]
 *
 * This class computes the atmospheric transmissivity η_atm of an FSO link.
 * This class is used inside the QdlFsoPropagationLossModel, where it contributes in the computation of the overall
 * transmissivity of the FSO link.
 *
 * It is an abstract class, as it contains the pure virtual DoGetAtmosphericTransmissivity().
 * The classes that implement it, model the atmopheric effetcs the with a varying level of complexity:
 *  1) Linear attenuation models: Kim and Kruse [Nebuloni2022]
 *  2) Numerical models: Lowtran [Kneizys1988]
 *
 * @see [Morellato2027] B. Morellato, R. Di Bella et al., "An FSO Channel Model for ns-3"
 * @see [Nebuloni2022] R. Nebuloni and E. Verdugo, "FSO Path Loss Model Based on the Visibility"
 * @see [Kneizys1988] F. X. Kneizys, et al., "Users guide to LOWTRAN 7"
 */
class QdlFsoAtmosphericModel : public Object
{
  public:
	static TypeId GetTypeId();

	QdlFsoAtmosphericModel();
	~QdlFsoAtmosphericModel() override;

	/**
	 * @brief Compute the (linear, dimensionless) atmospheric transmissivity
	 *        η_atm in [0,1] for the given channel context.
	 *
	 * @param context Channel/link geometry and physical parameters.
	 * @return η_atm, the linear atmospheric transmissivity.
	 */
	double GetAtmosphericTransmissivity(const QdlFsoPropagationContext& ctx);

  protected:
	// inherited from Object
	void DoDispose() override;

  private:
	/**
	 * @brief Pure virtual method for subclasses to compute the atmospheric transmissivity.
	 *
	 * @param context Channel/link geometry and physical parameters.
	 * @return eta_atm, the linear atmospheric transmissivity.
	 */
	virtual double DoGetAtmosphericTransmissivity(const QdlFsoPropagationContext& ctx) = 0;
};

/**
 * @ingroup qdl-fso
 * @brief Kim Atmospheric Model for the qdl-fso module
 *
 * Uses the Kim model of [Nebuloni2022] to model the atmospheric attenuation.
 * This model can be used for links that don't cross multiple atmospheric regions.
 * It implements the abstract class QdlFsoAtmosphericModel.
 *
 * @see [Nebuloni2022] R. Nebuloni and E. Verdugo, "FSO Path Loss Model Based on the Visibility"
 */
class QdlFsoKimAtmosphericModel : public QdlFsoAtmosphericModel
{
  public:
	/**
	 * @brief Get the type ID.
	 * @return the object TypeId
	 */
	static TypeId GetTypeId();

	/**
	 * @brief QdlFsoKimAtmosphericModel constructor
	 */
	QdlFsoKimAtmosphericModel();

	/**
	 * @brief ~QdlFsoKimAtmosphericModel destructor
	 */
	~QdlFsoKimAtmosphericModel() override;

  private:
	/**
	 * @brief Compute the (linear, dimensionless) atmospheric transmissivity
	 *        η_atm in [0,1] for the given channel context.
	 *
	 * @param context Channel/link geometry and physical parameters.
	 * @return η_atm, the linear atmospheric transmissivity.
	 */
	double DoGetAtmosphericTransmissivity(const QdlFsoPropagationContext& context);

	/**
	 * @brief Compute the exponent p for the given channel condition.
	 *
	 * @param visibilityKm Visibility of the link in km, set according to [ITU2012].
	 * @return p, the exponent.
	 *
	 * @see [ITU2012] ITU, "RECOMMENDATION ITU-R P.1817-1 - Propagation data required for the design of terrestrial
	 * free-space optical links"
	 */
	double GetExponent(double visibilityKm) const;

	/**
	 * @brief Compute the atmospheric attenuation β_db in dB/km for the given channel condition.
	 *
	 * @param visibilityKm Visibility of the link in km, set according to [ITU2012].
	 * @param wavelengthNm Wavelength of the channel, in nm.
	 * @return β_db, the attenuation of the link, in dB/km.
	 *
	 * @see [ITU2012] ITU, "RECOMMENDATION ITU-R P.1817-1 - Propagation data required for the design of terrestrial
	 * free-space optical links"
	 */
	double GetAtmosphericAttenuationDbPerKm(double visibilityKm, double wavelengthNm);

	/**
	 * @brief Compute the rain attenuation γ_rain in dB/km.
	 *
	 * Uses the power-law model γ_rain = k * R^α, where R is the rain rate in mm/h.
	 * At optical wavelengths the raindrops are much larger than the wavelength, so the
	 * attenuation is considered wavelength-independent.
	 * Returns 0 if the rain rate is not positive (rain attenuation disabled).
	 *
	 * @note Rain also reduces the visibility. To avoid counting the same loss twice, set
	 * m_visibilityKm to the visibility of the air without rain, or, if it comes from a
	 * measurement taken during rain, leave the rain rate at 0.
	 *
	 * @return γ_rain, the rain attenuation of the link, in dB/km.
	 */
	double GetRainAttenuationDbPerKm() const;

	double m_visibilityKm;				   //!< Meteorological visibility in kilometers (V)
	double m_additionalAttenuationDbPerKm; //!< Additional attenuation in dB/km (could be snow, etc..)
	double m_rainRateMmPerHour;			   //!< Rain rate R in mm/h (0 disables rain attenuation)
	double m_rainCoeffK;				   //!< Coefficient k of γ_rain = k * R^α, in dB/km (default: Carbonneau, 1.076)
	double m_rainExponentAlpha;			   //!< Exponent α of γ_rain = k * R^α (default: Carbonneau, 0.67)
};

/**
 * @ingroup qdl-fso
 * @brief Kruse Atmospheric Model for the qdl-fso module
 *
 * Uses the Kruse model of [Nebuloni2022] to model the atmospheric attenuation.
 * This model can be used for terrestrial scenarios only.
 * It implements the abstract class QdlFsoAtmosphericModel.
 *
 * @see [Nebuloni2022] R. Nebuloni and E. Verdugo, "FSO Path Loss Model Based on the Visibility"
 */
class QdlFsoKruseAtmosphericModel : public QdlFsoAtmosphericModel
{
  public:
	/**
	 * @brief Get the type ID.
	 * @return the object TypeId
	 */
	static TypeId GetTypeId();

	/**
	 * @brief QdlFsoKruseAtmosphericModel constructor
	 */
	QdlFsoKruseAtmosphericModel();

	/**
	 * @brief ~QdlFsoKruseAtmosphericModel destructor
	 */
	~QdlFsoKruseAtmosphericModel() override;

  private:
	/**
	 * @brief Compute the (linear, dimensionless) atmospheric transmissivity
	 *        η_atm in [0,1] for the given channel context.
	 *
	 * @param context Channel/link geometry and physical parameters.
	 * @return η_atm, the linear atmospheric transmissivity.
	 */
	double DoGetAtmosphericTransmissivity(const QdlFsoPropagationContext& context);

	/**
	 * @brief Compute the exponent p for the given channel condition.
	 *
	 * @param visibilityKm Visibility of the link in km, set according to [ITU2012].
	 * @return p, the exponent.
	 *
	 * @see [ITU2012] ITU, "RECOMMENDATION ITU-R P.1817-1 - Propagation data required for the design of terrestrial
	 * free-space optical links"
	 */
	double GetExponent(double visibilityKm) const;

	/**
	 * @brief Compute the atmospheric attenuation β_db in dB/km for the given channel condition.
	 *
	 * @param visibilityKm Visibility of the link in km, set according to [ITU2012].
	 * @param wavelengthNm Wavelength of the channel, in nm.
	 * @return β_db, the attenuation of the link, in dB/km.
	 *
	 * @see [ITU2012] ITU, "RECOMMENDATION ITU-R P.1817-1 - Propagation data required for the design of terrestrial
	 * free-space optical links"
	 */
	double GetAtmosphericAttenuationDbPerKm(double visibilityKm, double wavelengthNm);

	/**
	 * @brief Compute the rain attenuation γ_rain in dB/km.
	 *
	 * Uses the power-law model γ_rain = k * R^α, where R is the rain rate in mm/h.
	 * At optical wavelengths the raindrops are much larger than the wavelength, so the
	 * attenuation is considered wavelength-independent.
	 * Returns 0 if the rain rate is not positive (rain attenuation disabled).
	 *
	 * @note Rain also reduces the visibility. To avoid counting the same loss twice, set
	 * m_visibilityKm to the visibility of the air without rain, or, if it comes from a
	 * measurement taken during rain, leave the rain rate at 0.
	 *
	 * @return γ_rain, the rain attenuation of the link, in dB/km.
	 */
	double GetRainAttenuationDbPerKm() const;

	double m_visibilityKm;				   //!< Meteorological visibility in kilometers (V)
	double m_additionalAttenuationDbPerKm; //!< Additional attenuation in dB/km (could be snow, etc..)
	double m_rainRateMmPerHour;			   //!< Rain rate R in mm/h (0 disables rain attenuation)
	double m_rainCoeffK;				   //!< Coefficient k of γ_rain = k * R^α, in dB/km (default: Carbonneau, 1.076)
	double m_rainExponentAlpha;			   //!< Exponent α of γ_rain = k * R^α (default: Carbonneau, 0.67)
};

/**
 * @ingroup qdl-fso
 * @brief Lowtran Atmospheric Model for the qdl-fso module
 *
 * Uses the Lowtran model of [Kneizys1988] to model the atmospheric attenuation.
 * This model can be used for all scenarios.
 * It implements the abstract class QdlFsoAtmosphericModel.
 *
 * @see [Kneizys1988] F. X. Kneizys, et al., "Users guide to LOWTRAN 7"
 */
class QdlFsoLowtranAtmosphericModel : public QdlFsoAtmosphericModel
{
  public:
	/**
	 * @brief Get the type ID.
	 * @return the object TypeId
	 */
	static TypeId GetTypeId();

	/**
	 * @brief QdlFsoLowtranAtmosphericModel constructor
	 */
	QdlFsoLowtranAtmosphericModel();

	/**
	 * @brief ~QdlFsoLowtranAtmosphericModel destructor
	 */
	~QdlFsoLowtranAtmosphericModel() override;

	enum class AtmosphericModel : int
	{
		/*
			C             'MODEL' SELECTS ONE OF SIX GEOGRAPHICAL MODEL ATMOSPHERES
			C              OR SPECIFIES THAT USER-DEFINED METEOROLOGICAL
			C              DATA ARE TO BE USED.
			C
			C
			C     MODEL=0 IF METEOROLOGICAL DATA ARE SPECIFIED(HORIZONTAL PATH ONLY)
			C           1 TROPICAL ATMOSPHERE
			C           2 MIDLATITUDE SUMMER
			C           3 MIDLATITUDE WINTER
			C           4 SUBARCTIC   SUMMER
			C           5 SUBARCTIC   WINTER
			C           6 1976 U.S. STANDARD ATMOSPHERE
			C           7 IF A NEW MODEL ATMOSPHERE( OR RADIOSONDE DATA) IS TO BE
			C             READ IN.
		*/
		TROPICAL = 1,
		MIDLATITUDE_SUMMER = 2,
		MIDLATITUDE_WINTER = 3,
		SUBARCTIC_SUMMER = 4,
		SUBARCTIC_WINTER = 5,
		US_STANDARD = 6,
	};

	enum class AerosolType : int
	{
		/*
			C     'IHAZE' SELECTS THE TYPE OF EXTINCTION AND A DEFAULT
			C     METEOROLOGICAL RANGE FOR THE BOUNDARY-LAYER AEROSOL MODEL
			C     (0 TO 2KM ALTITUDE)
			C     IF 'VIS' IS ALSO SPECIFIED ON CARD 2 IT WILL OVERRIDE THE
			C     DEFAULT 'IHAZE' VALUE  FOR METEOROLOGICAL RANGE
			C
			C     IHAZE=0  NO AEROSOL ATTENUATION INCLUDED IN CALCULATION.
			C          =1  RURAL EXTINCTION, 23-KM VIS.
			C          =2  RURAL EXTINCTION, 5-KM VIS.
			C          =3  NAVY MARITIME EXTINCTION,SETS OWN VIS.
			C          =4  MARITIME EXTINCTION, 23-KM VIS.    (LOWTRAN 5 MODEL)
			C          =5  URBAN EXTINCTION, 5-KM VIS.
			C          =6  TROPOSPHERIC EXTINCTION, 50-KM VIS.
			C          =7  USER DEFINED  AEROSOL EXTINCTION COEFFICIENTS
			C              TRIGGERS READING IREG FOR UP TO 4 REGIONS OF
			C              USER DEFINED EXTINCTION ABSORPTION AND ASYMMETRY
			C          =8  FOG1 (ADVECTIVE FOG) EXTINCTION, 0.2-KM VIS.
			C          =9  FOG2 (RADIATIVE FOG) EXTINCTION, 0.5-KM VIS.
			C          =10 DESERT EXTINCTION  SETS OWN VISIBILITY FROM WIND SPEED
		*/
		NO_AEROSOL = 0,
		RURAL_23_KM = 1,
		RURAL_5_KM = 2,
		NAVY = 3,
		MARITIME_23_KM = 4,
		URBAN_5_KM = 5,
		TROPO_50_KM = 6,
		FOG_200_M = 8,
		FOG_500_M = 9,
		DESERT = 10,
	};

	enum class Season : int
	{
		/*
			C     'ISEASN' SELECTS THE SEASONAL DEPENDENCE OF THE PROFILES
			C     FOR BOTH THE TROPOSPHERIC (2 TO 10 KM) AND
			C     STRATOSPHERIC(10 TO 30 KM) AEROSOLS.
			C
			C     ISEASN=0 DEFAULTS TO SEASON OF 'MODEL'
			C              (MODEL 0,1,2,4,6,7) SUMMER
			C              (MODEL 3,5)         WINTER
			C           =1 SPRING-SUMMER
			C           =2 FALL - WINTER
		*/
		USE_DEFAULT = 0,
		SPRING_SUMMER = 1,
		FALL_WINTER = 2,
	};

	// Note: VolcanicTypeProfile has no impact if AerosolType==NO_AEROSOL
	enum class VolcanicTypeProfile : int
	{
		/*
			C     'IVULCN' SELECTS BOTH THE PROFILE AND EXTINCTION TYPE
			C     FOR THE STRATOSPHERIC AEROSOLS AND DETERMINES TRANSITION
			C     PROFILES ABOVE THE STRATOSPHERE TO 100 KM.
			C
			C     IVULCN=0 DEFAULT TO STRATOSPHERIC BACKGROUND
			C           =1 STRATOSPHERIC BACKGROUND
			C           =2 AGED VOLCANIC TYPE/MODERATE VOLCANIC PROFILE
			C           =3 FRESH VOLCANIC TYPE/HIGH VOLCANIC PROFILE
			C           =4 AGED VOLCANIC TYPE/HIGH VOLCANIC PROFILE
			C           =5 FRESH VOLCANIC TYPE/MODERATE VOLCANIC PROFILE
			C           =6 BACKGROUND STRATOSPHERIC TYPE/MODERATE VOLCANIC PROFILE
			C           =7 BACKGROUND STRATOSPHERIC TYPE/HIGH VOLCANIC PROFILE
			C           =8 FRESH VOLCANIC TYPE/EXTREME VOLCANIC PROFILE
		*/
		DEFAULT = 0,
		STRATOSPHERIC_BACKGROUND = 1,
		AGED_VOLC_MODERATE_VOLC = 2,
		FRESH_VOLC_HIGH_VOLC = 3,
		AGED_VOLC_HIGH_VOLC = 4,
		FRESH_VOLC_MODERATE_VOLC = 5,
		BACKGROUND_STRATO_MODERATE_VOLC = 6,
		BACKGROUND_STRATO_HIGH_VOLC = 7,
		FRESH_VOLC_EXTREME_VOLC = 8,
	};

	enum class CloudRainModel : int
	{
		/*
			C     'ICLD' SPECIFIES WHICH OF THE CLOUD MODELS AND THE RAIN RATES
			C     ARE USED
			C
			C     ICLD  FOR CLOUD AND OR RAIN
			C     ICLD = 0   NO CLOUDS OR RAIN
			C          = 1  CUMULUS CLOUD BASE .66KM TOP 2.7KM
			C          = 2  ALTOSTRATUS CLOUD BASE 2.4KM TOP 3.0KM
			C          = 3  STRATUS CLOUD BASE .33KM TOP 1.0KM
			C          = 4  STRATUS/STRATO CU BASE .66KM TOP 2.0KM
			C          = 5  NIMBOSTRATUS CLOUD BASE .16KM TOP .66KM
			C          = 6  2.0MM/HR DRIZZLE (MODELED WITH CLOUD  3)
			C               RAIN  2. MM HR AT 0KM TO .22 MM HR AT 1.5KM
			C          = 7  5.0MM/HR LIGHT RAIN (MODELED WITH CLOUD  5)
			C               RAIN  5. MM HR AT 0KM TO .2  MM HR AT 1.5KM
			C          = 8  12.5MM/HR MODERATE RAIN (MODELED WITH CLOUD  5)
			C               RAIN 12.5MM HR AT 0KM TO .2  MM HR AT 2.0KM
			C          = 9  25.0MM/HR HEAVY RAIN (MODELED WITH CLOUD  1)
			C               RAIN 25. MM HR AT 0KM TO .2  MM HR AT 3.0KM
			C          =10  75.0MM/HR EXTREME RAIN (MODELED WITH CLOUD  1)
			C               RAIN 75. MM HR AT 0KM TO .2  MM HR AT 3.5KM
			C          =11  READ IN USER DEFINED CLOUD EXTINCTION AND ABSORPTION
			C              USER DEFINED  AEROSOL EXTINCTION COEFFICIENTS
			C              TRIGGERS READING IREG FOR UP TO 4 REGIONS OF
			C              USER DEFINED EXTINCTION ABSORPTION AND ASYMMETRY
			C          =18  STANDARD   CIRRUS MODEL
			C          =19  SUB VISUAL CIRRUS MODEL
			C          =20  NOAA       CIRRUS MODEL  (LOWTRAN 6 MODEL)
		*/
		NO_CLOUDS = 0,
		CUMULUS_660_M_2_7_KM = 1,
		ALTOSTRATUS_2_4_KM_3_KM = 2,
		STRATUS_330_M_1_KM = 3,
		STRATUS_660_M_1_KM = 4,
		NIMBOSTRATUS_160_M_660_M = 5,
		DRIZZLE_2MM_HR = 6,
		LIGHT_RAIN_5MM_HR = 7,
		MODERATE_RAIN_12_5MM_HR = 8,
		HEAVY_RAIN_25MM_HR = 9,
		EXTREME_RAIN_75MM_HR = 10,
#ifdef ENABLE_LOWTRAN_CIRRUS_MODELS
		// These 3 options are disabled by default because they seem to give gibberish results
		STD_CIRRUS_MODEL = 18,
		SUB_VISUAL_CIRRUS_MODEL = 19,
		NOAA_CIRRUS_MODEL = 20,
#endif
	};

  private:
	/**
	 * @brief Compute the (linear, dimensionless) atmospheric transmissivity
	 *        η_atm in [0,1] for the given channel context.
	 *
	 * @param context Channel/link geometry and physical parameters.
	 * @return η_atm, the linear atmospheric transmissivity.
	 */
	double DoGetAtmosphericTransmissivity(const QdlFsoPropagationContext& context);

	float m_visibility;		// [km]
	float m_groundAltitude; // [km]
	float m_windSpeed;		// [m/s]
	float m_avgWindSpeed;	// [m/s]
	float m_rainRate;		// [mm/hr]
	int m_airMassCharacter; // 0 to 10, used only with the Navy Maritime atmospheric model
	AtmosphericModel m_atmosphericModel;
	AerosolType m_aerosolType;
	Season m_season;
	CloudRainModel m_couldRainModel;
	VolcanicTypeProfile m_volcanicTypeProfile;

	enum class PathType : int
	{
		/*
			C           'ITYPE' INDICATES THE TYPE OF ATMOSPHERIC PATH
			C
			C     ITYPE=1 FOR A HORIZONTAL (CONSTANT-PRESSURE) PATH
			C           2 VERTICAL OR SLANT PATH BETWEEN TWO ALTITUDES
			C           3 FOR A VERTICAL OR SLANT PATH TO SPACE
		*/
		HORIZONTAL = 1,
		SLANT_ATMOSPHERIC = 2,
		SLANT_TO_SPACE = 3
	};

	/**
	 * @brief Pass the given parameters to Lowtran and returns the computed transmissivity
	 *
	 * The optional return type is used to handle gracefully cases in which Lowtran does not accept
	 * the given combination of input parameters, without raising exceptions that need to be handled
	 * elsewhere.
	 *
	 * The version of LOWTRAN compiled together with qdlfso has been modified to avoid anything being
	 * printed to stdout during normal operation. However, it may be possible that it still outputs
	 * warning or error messages to stdout in case of out-of-range or invalid parameters. If this is
	 * to be avoided, defining the DISABLE_FORTRAN_STDOUT macro will disable LOWTRAN output.
	 *
	 * @param wavelength_nm Wavelength of the optical carrier, in nm.
	 * @param model Geographical atmospheric model (tropical, midlatitude summer/winter, subarctic summer/winter,
	 *              US standard 1976).
	 * @param itype Type of atmospheric path: horizontal, slant between two altitudes, or slant to space.
	 * @param h1 Altitude of the lower end of the path, in km.
	 * @param h2 Altitude of the upper end of the path, in km.
	 * @param angle Zenith angle of the path, in degrees (0 to 90).
	 * @param range_km Length of the (slant) path, in km.
	 * @param ihaze Aerosol/haze extinction model used for the boundary layer (0 to 2 km altitude).
	 * @param vis_km Surface meteorological visibility, in km. If <= 0, Lowtran uses the default visibility
	 *               associated with the selected aerosol model.
	 * @param iseasn Season used for the tropospheric and stratospheric aerosol profiles
	 *               (default of the atmospheric model, spring-summer, or fall-winter).
	 * @param icstl Air mass character, from 1 (open ocean) to 10 (strong continental influence).
	 *              Used only with the Navy Maritime aerosol model.
	 * @param icld Cloud/rain model.
	 * @param ivulcn Volcanic aerosol profile and extinction type for the stratosphere.
	 * @param wss Current wind speed, in m/s. Used only with the Navy Maritime and Desert aerosol models.
	 * @param whh 24-hour average wind speed, in m/s. Used only with the Navy Maritime aerosol model.
	 * @param rain_rate Rain rate, in mm/hr.
	 * @param gndalt_km Altitude of the ground above sea level, in km.
	 * @return The linear atmospheric transmissivity in [0, 1] computed by Lowtran.
	 * @retval std::nullopt Lowtran failed or returned an invalid result (see above).
	 */
	std::optional<float> lowtran_cpp_wrapper(float wavelength_nm,
											 AtmosphericModel model,
											 PathType itype,
											 float h1,
											 float h2,
											 float angle,
											 float range_km,
											 AerosolType ihaze,
											 float vis_km,
											 Season iseasn,
											 int icstl,
											 CloudRainModel icld,
											 VolcanicTypeProfile ivulcn,
											 float wss,
											 float whh,
											 float rain_rate,
											 float gndalt_km);
};

} // namespace ns3

#endif // QDL_FSO_ATMOSPHERIC_MODEL_H
