module lowtran_c_interface
use iso_c_binding
implicit none

contains

subroutine lowtran_transmittance(wavelength_nm, model, itype, &
    h1, h2, angle, range_km, &
    ihaze, vis_km, iseasn, icstl, icld, ivulcn, wss, whh, rain_rate, gndalt_km, &
    transmittance, wavelength_out_nm) &
    bind(C, name="lowtran_transmittance")

! ---- C-visible interface: all scalars passed BY VALUE ----
real(c_float),  value       :: wavelength_nm   ! input wavelength [nm]
integer(c_int), value       :: model            ! atmosphere model (1-7)
integer(c_int), value       :: itype            ! path type (1-3)
real(c_float),  value       :: h1, h2           ! altitudes [km]
real(c_float),  value       :: angle            ! zenith angle [deg]
real(c_float),  value       :: range_km         ! slant range [km]

integer(c_int), value       :: ihaze            ! aerosol/haze model (0-10)
real(c_float),  value       :: vis_km           ! surface visibility [km], <=0 => IHAZE default
integer(c_int), value       :: iseasn           ! season (0=default,1=spring/summer,2=fall/winter)
integer(c_int), value       :: icstl            ! air-mass character (1-10), only used if IHAZE selects maritime
integer(c_int), value       :: icld             ! cloud/rain model (0-19; avoid 18-20, CARD2A not wired)
integer(c_int), value       :: ivulcn           ! volcanic aerosol model (0-7)
real(c_float),  value       :: wss              ! wind speed [m/s] needed for ihaze=3 and ihaze=10 (navy and desert)
real(c_float),  value       :: whh              ! 24h average wind speed [m/s] for ihaze=3 (navy)
real(c_float),  value       :: rain_rate        ! rain rate [mm/hr]
real(c_float),  value       :: gndalt_km        ! ground altitude [km]

real(c_float),  intent(out) :: transmittance    ! result
real(c_float),  intent(out) :: wavelength_out_nm

integer, parameter :: nwl = 1
integer, parameter :: ml  = 1

real :: v1, v2, dv
real :: tx(nwl,63)
real :: v(nwl), alam(nwl)
real :: trace(nwl), unif(nwl)
real :: suma(nwl)
real :: irrad(nwl,3)
real :: sumvv(nwl)

integer :: iemsct, im, ird1
real :: zmdl(ml), p(ml), t(ml), wmol(12)

    v1 = 1.0e7 / wavelength_nm
    v2 = v1
    dv = 20.0

    iemsct = 0   ! transmittance-only
    im     = 0
    ird1   = 0

    zmdl = 0.0
    p    = 0.0
    t    = 0.0
    wmol = 0.0

    ! NOTE: matches lowtran7_fixed.f's LWTRN7 argument order exactly:
    ! Python,nwl,V1,V2,DV, TX,V,ALAM,TRACE,UNIF,SUMA,IRRAD,SUMVV,
    ! MODEL,ITYPE,IEMSCT,IM, IHAZE,ISEASN,IVULCN,ICSTL,ICLD,ML,IRD1,
    ! ZMDL,P,T,WMOL, H1,H2,ANGLE,RANGE, VIS,GNDALT
    call lwtrn7(.true., nwl, v1, v2, dv, &
                tx, v, alam, trace, unif, &
                suma, irrad, sumvv, &
                model, itype, iemsct, im, &
                ihaze, iseasn, ivulcn, icstl, icld, ml, ird1, &
                zmdl, p, t, wmol, &
                h1, h2, angle, range_km, &
                vis_km, wss, whh, rain_rate, gndalt_km)

    transmittance = tx(1,9)
    wavelength_out_nm = alam(1) * 1000.0

end subroutine lowtran_transmittance

end module lowtran_c_interface