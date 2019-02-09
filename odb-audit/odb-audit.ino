#include <ILI9341_t3.h>
#include <OBD2UART.h>

COBD obd;

#define TFT_DC  9
#define TFT_CS 10
ILI9341_t3 tft = ILI9341_t3(TFT_CS, TFT_DC);

static byte PIDS[] = {
    PID_ENGINE_LOAD,
    PID_COOLANT_TEMP,
    PID_SHORT_TERM_FUEL_TRIM_1,
    PID_LONG_TERM_FUEL_TRIM_1,
    PID_SHORT_TERM_FUEL_TRIM_2,
    PID_LONG_TERM_FUEL_TRIM_2,
    PID_FUEL_PRESSURE,
    PID_INTAKE_MAP,
    PID_RPM,
    PID_SPEED,
    PID_TIMING_ADVANCE,
    PID_INTAKE_TEMP,
    PID_MAF_FLOW,
    PID_THROTTLE,
    PID_AUX_INPUT,
    PID_RUNTIME,
    PID_DISTANCE_WITH_MIL,
    PID_COMMANDED_EGR,
    PID_EGR_ERROR,
    PID_COMMANDED_EVAPORATIVE_PURGE,
    PID_FUEL_LEVEL,
    PID_WARMS_UPS,
    PID_DISTANCE,
    PID_EVAP_SYS_VAPOR_PRESSURE,
    PID_BAROMETRIC,
    PID_CATALYST_TEMP_B1S1,
    PID_CATALYST_TEMP_B2S1,
    PID_CATALYST_TEMP_B1S2,
    PID_CATALYST_TEMP_B2S2,
    PID_CONTROL_MODULE_VOLTAGE,
    PID_ABSOLUTE_ENGINE_LOAD,
    PID_AIR_FUEL_EQUIV_RATIO,
    PID_RELATIVE_THROTTLE_POS,
    PID_AMBIENT_TEMP,
    PID_ABSOLUTE_THROTTLE_POS_B,
    PID_ABSOLUTE_THROTTLE_POS_C,
    PID_ACC_PEDAL_POS_D,
    PID_ACC_PEDAL_POS_E,
    PID_ACC_PEDAL_POS_F,
    PID_COMMANDED_THROTTLE_ACTUATOR,
    PID_TIME_WITH_MIL,
    PID_TIME_SINCE_CODES_CLEARED,
    PID_ETHANOL_FUEL,
    PID_FUEL_RAIL_PRESSURE,
    PID_HYBRID_BATTERY_PERCENTAGE,
    PID_ENGINE_OIL_TEMP,
    PID_FUEL_INJECTION_TIMING,
    PID_ENGINE_FUEL_RATE,
    PID_ENGINE_TORQUE_DEMANDED,
    PID_ENGINE_TORQUE_PERCENTAGE,
    PID_ENGINE_REF_TORQUE,
    PID_ACC,
    PID_GYRO,
    PID_COMPASS,
    PID_MEMS_TEMP,
    PID_BATTERY_VOLTAGE
};

static const char* PIDNames[] = {
    "ENGINE_LOAD",
    "COOLANT_TEMP",
    "SHORT_TERM_FUEL_TRIM_1",
    "LONG_TERM_FUEL_TRIM_1",
    "SHORT_TERM_FUEL_TRIM_2",
    "LONG_TERM_FUEL_TRIM_2",
    "FUEL_PRESSURE",
    "INTAKE_MAP",
    "RPM",
    "SPEED",
    "TIMING_ADVANCE",
    "INTAKE_TEMP",
    "MAF_FLOW",
    "THROTTLE",
    "AUX_INPUT",
    "RUNTIME",
    "DISTANCE_WITH_MIL",
    "COMMANDED_EGR",
    "EGR_ERROR",
    "FUEL_LEVEL",
    "WARMS_UPS",
    "DISTANCE",
    "BAROMETRIC",
    //long words here
    "EVAP_SYS_VAPOR_PRESSURE",
    "COMMANDED_EVAPORATIVE_PURGE",
    "HYBRID_BATTERY_PERCENTAGE",
    "ABSOLUTE_THROTTLE_POS_B",
    "ABSOLUTE_THROTTLE_POS_C",
    "COMMANDED_THROTTLE_ACTUATOR",
    //
    "RELATIVE_THROTTLE_POS",
    "CATALYST_TEMP_B1S1",
    "CATALYST_TEMP_B2S1",
    "CATALYST_TEMP_B1S2",
    "CATALYST_TEMP_B2S2",
    "CONTROL_MODULE_VOLTAGE",
    "ABSOLUTE_ENGINE_LOAD",
    "AIR_FUEL_EQUIV_RATIO",
    "AMBIENT_TEMP",
    "ACC_PEDAL_POS_D",
    "ACC_PEDAL_POS_E",
    "ACC_PEDAL_POS_F",
    "TIME_WITH_MIL",
    "TIME_SINCE_CODES_CLEARED",
    "ETHANOL_FUEL",
    "FUEL_RAIL_PRESSURE",
    "ENGINE_OIL_TEMP",
    "FUEL_INJECTION_TIMING",
    "ENGINE_FUEL_RATE",
    "ENGINE_TORQUE_DEMANDED",
    "ENGINE_TORQUE_PERCENTAGE",
    "ENGINE_REF_TORQUE",
    "ACC",
    "GYRO",
    "COMPASS",
    "MEMS_TEMP",
    "BATTERY_VOLTAGE"
};

void setup(void) {
    //display init
    tft.begin();  
    tft.setRotation(1);
    tft.fillScreen(ILI9341_RED);

    delay(2000);
    obd.begin();
    while (!obd.init()); 

    for (int i = 0; i < 56; i++) {
        if (i > 30) {
            tft.setCursor(320 / 2, (i - 31) * 8);
        }
        tft.print(PIDNames[i]);
        tft.print(" ");
        tft.println(obd.isValidPID(PIDS[i]));
    }
}

void loop(void) {

}